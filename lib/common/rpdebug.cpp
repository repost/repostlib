
#include "rpversion.h"
#include "rpdebug.h"
#include <iomanip>
#include <vector>

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

using namespace std;

static bool IsRepostLoggingRunning = false;
static LogSeverity  RepostLoggingLevel = DEBUG;
static vector<LogSink*> *RepostLogSinks = NULL;

const char*const LogSeverityNames[NUM_SEVERITIES] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

LogMessage::LogMessage(const char* file, const char* func, int line, LogSeverity severity) 
{
    Init(file, func, line, severity);
}

LogMessage::LogMessageData::~LogMessageData() {
  delete stream_;
}

const char* const_basename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
#ifdef WIN32  /* Look for either path separator in Windows */
  if (!base)
    base = strrchr(filepath, '\\');
#endif
  return base ? (base+1) : filepath;
}

void LogMessage::Init(const char* file,
                      const char* func,
                      int line,
                      LogSeverity severity)
{
    allocated_ = new LogMessageData();
    data_ = allocated_;
    data_->stream_= new LogStream(0);

    stream().fill('0');
    data_->severity_ = severity;
    data_->line_ = line;
    data_->timestamp_ = time( NULL );
    localtime_r(&data_->timestamp_, &data_->tm_time_);
    data_->basename_ = const_basename(file);
    data_->fullname_ = file;
    data_->has_been_flushed_ = false;
    data_->func_ = func;

    /* 
    ** If specified, prepend a prefix to each line.  For example:
    ** I1018 16:07:15 logger:logging.cc:1153]
    ** (log level, GMT month, date, time, func, file basename, line)
    */
        stream() << LogSeverityNames[severity][0]
            << setw(2) << 1+data_->tm_time_.tm_mon
            << setw(2) << data_->tm_time_.tm_mday
            << ' '
            << setw(2) << data_->tm_time_.tm_hour  << ':'
            << setw(2) << data_->tm_time_.tm_min   << ':'
            << setw(2) << data_->tm_time_.tm_sec   << " "
            << data_->basename_ << ':' << data_->line_ << "] ";
}

LogMessage::~LogMessage() 
{
  Flush();
  delete allocated_;
}

/* 
** Flush buffered message, called by the destructor, or any other function
** that needs to synchronize the log.
*/
void LogMessage::Flush() 
{
    if (data_->has_been_flushed_ || 
            data_->severity_ < RepostLoggingLevel) 
    {
        return;
    }

    vector<LogSink*>::iterator sink;
    for ( sink = RepostLogSinks->begin() ; sink < RepostLogSinks->end(); sink++ )
    {
        (*sink)->send(data_->severity_, stream().str());
    }
    data_->has_been_flushed_ = true;
}

void InitRepostLogging(void)
{
    if(IsRepostLoggingRunning == false)
    {
        RepostLogSinks = new vector<LogSink*>;
        LogStdoutSink* StdoutSink = new LogStdoutSink();
        RepostLogSinks->push_back(StdoutSink);

        /* initialisation here */
        LOG(INFO) <<"\n"
                    "                           _   \n"  
                    "                          | |  \n"  
                    "  _ __ ___ _ __   ___  ___| |_ \n"
                    " | '__/ _ \\ '_ \\ / _ \\/ __| __|\n"
                    " | | |  __/ |_) | (_) \\__ \\ |_ \n"
                    " |_|  \\___| .__/ \\___/|___/\\__|\n"
                    "          | |                  \n"
                    "          |_|                  \n"
                    " " RP_VERSION_STRING "\n"
                    " " RP_RELEASE_TAGLINE "\n";
        IsRepostLoggingRunning = true;
    }
}

void SetRepostLogLevel(LogSeverity severity)
{
    RepostLoggingLevel = severity;
}

void LogStdoutSink::send(LogSeverity severity, std::string msg)
{
    cout << msg << endl;
}

LogFileSink::LogFileSink(LogSeverity severity,
                             const char* base_filename)
  : base_filename_selected_(base_filename != NULL),
    base_filename_((base_filename != NULL) ? base_filename : ""),
    file_(NULL),
    severity_(severity),
    bytes_since_flush_(0),
    file_length_(0),
    rollover_attempt_(kRolloverAttemptFrequency-1),
    next_flush_time_(0) {
#ifdef NAMED_SEMAPHORE
    char name[32];
    long now;
    now = (long) time(0);
    snprintf(name, 32, "/%s-%10d-%10ld", "boss", getpid(), now);
    lock_ = sem_open(name, O_CREAT, 0, 1);
#else
    lock_ = new sem_t;
    sem_init(lock_, 0, 1);
#endif
}

LogFileObject::~LogFileObject() {
    sem_wait(lock_);
    if (file_ != NULL) {
        fclose(file_);
        file_ = NULL;
    }
#ifdef NAMED_SEMAPHORE
    sem_close(boss);
    sem_close(spinner);
#else
    sem_destroy(boss);
    sem_destroy(spinner);
#endif
}

void LogFileObject::SetBasename(const char* basename) {
  MutexLock l(&lock_);
    sem_wait(lock_);
  base_filename_selected_ = true;
  if (base_filename_ != basename) {
    /* Get rid of old log file since we are changing names */
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
      rollover_attempt_ = kRolloverAttemptFrequency-1;
    }
    base_filename_ = basename;
  }
}

void LogFileObject::SetSymlinkBasename(const char* symlink_basename) {
  MutexLock l(&lock_);
  symlink_basename_ = symlink_basename;
}

void LogFileObject::Flush() {
  MutexLock l(&lock_);
  FlushUnlocked();
}

void LogFileObject::FlushUnlocked(){
  if (file_ != NULL) {
    fflush(file_);
    bytes_since_flush_ = 0;
  }
  // Figure out when we are due for another flush.
  const int64 next = (FLAGS_logbufsecs
                      * static_cast<int64>(1000000));  // in usec
  next_flush_time_ = CycleClock_Now() + UsecToCycles(next);
}

bool LogFileObject::CreateLogfile(const char* time_pid_string) {
  string string_filename = base_filename_+filename_extension_+
                           time_pid_string;
  const char* filename = string_filename.c_str();
  int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0664);
  if (fd == -1) return false;
#ifdef HAVE_FCNTL
  // Mark the file close-on-exec. We don't really care if this fails
  fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

  file_ = fdopen(fd, "a");  // Make a FILE*.
  if (file_ == NULL) {  // Man, we're screwed!
    close(fd);
    unlink(filename);  // Erase the half-baked evidence: an unusable log file
    return false;
  }

  // We try to create a symlink called <program_name>.<severity>,
  // which is easier to use.  (Every time we create a new logfile,
  // we destroy the old symlink and create a new one, so it always
  // points to the latest logfile.)  If it fails, we're sad but it's
  // no error.
  if (!symlink_basename_.empty()) {
    // take directory from filename
    const char* slash = strrchr(filename, PATH_SEPARATOR);
    const string linkname =
      symlink_basename_ + '.' + LogSeverityNames[severity_];
    string linkpath;
    if ( slash ) linkpath = string(filename, slash-filename+1);  // get dirname
    linkpath += linkname;
    unlink(linkpath.c_str());                    // delete old one if it exists

    // We must have unistd.h.
#ifdef HAVE_UNISTD_H
    // Make the symlink be relative (in the same dir) so that if the
    // entire log directory gets relocated the link is still valid.
    const char *linkdest = slash ? (slash + 1) : filename;
    symlink(linkdest, linkpath.c_str());         // silently ignore failures

    // Make an additional link to the log file in a place specified by
    // FLAGS_log_link, if indicated
    if (!FLAGS_log_link.empty()) {
      linkpath = FLAGS_log_link + "/" + linkname;
      unlink(linkpath.c_str());                  // delete old one if it exists
      symlink(filename, linkpath.c_str());       // silently ignore failures
    }
#endif
  }

  return true;  // Everything worked
}

void LogFileObject::Write(bool force_flush,
                          time_t timestamp,
                          const char* message,
                          int message_len) {
  MutexLock l(&lock_);

  // We don't log if the base_name_ is "" (which means "don't write")
  if (base_filename_selected_ && base_filename_.empty()) {
    return;
  }

  if (static_cast<int>(file_length_ >> 20) >= MaxLogSize()) {
    if (file_ != NULL) fclose(file_);
    file_ = NULL;
    file_length_ = bytes_since_flush_ = 0;
    rollover_attempt_ = kRolloverAttemptFrequency-1;
  }

  // If there's no destination file, make one before outputting
  if (file_ == NULL) {
    // Try to rollover the log file every 32 log messages.  The only time
    // this could matter would be when we have trouble creating the log
    // file.  If that happens, we'll lose lots of log messages, of course!
    if (++rollover_attempt_ != kRolloverAttemptFrequency) return;
    rollover_attempt_ = 0;

    struct ::tm tm_time;
    localtime_r(&timestamp, &tm_time);

    // The logfile's filename will have the date/time & pid in it
    char time_pid_string[256];  // More than enough chars for time, pid, \0
    ostrstream time_pid_stream(time_pid_string, sizeof(time_pid_string));
    time_pid_stream.fill('0');
    time_pid_stream << 1900+tm_time.tm_year
		    << setw(2) << 1+tm_time.tm_mon
		    << setw(2) << tm_time.tm_mday
		    << '-'
		    << setw(2) << tm_time.tm_hour
		    << setw(2) << tm_time.tm_min
		    << setw(2) << tm_time.tm_sec
		    << '.'
		    << GetMainThreadPid()
		    << '\0';

    if (base_filename_selected_) {
      if (!CreateLogfile(time_pid_string)) {
        perror("Could not create log file");
        fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", time_pid_string);
        return;
      }
    } else {
      // If no base filename for logs of this severity has been set, use a
      // default base filename of
      // "<program name>.<hostname>.<user name>.log.<severity level>.".  So
      // logfiles will have names like
      // webserver.examplehost.root.log.INFO.19990817-150000.4354, where
      // 19990817 is a date (1999 August 17), 150000 is a time (15:00:00),
      // and 4354 is the pid of the logging process.  The date & time reflect
      // when the file was created for output.
      //
      // Where does the file get put?  Successively try the directories
      // "/tmp", and "."
      string stripped_filename(
          glog_internal_namespace_::ProgramInvocationShortName());
      string hostname;
      GetHostName(&hostname);

      string uidname = MyUserName();
      // We should not call CHECK() here because this function can be
      // called after holding on to log_mutex. We don't want to
      // attempt to hold on to the same mutex, and get into a
      // deadlock. Simply use a name like invalid-user.
      if (uidname.empty()) uidname = "invalid-user";

      stripped_filename = stripped_filename+'.'+hostname+'.'
                          +uidname+".log."
                          +LogSeverityNames[severity_]+'.';
      // We're going to (potentially) try to put logs in several different dirs
      const vector<string> & log_dirs = GetLoggingDirectories();

      // Go through the list of dirs, and try to create the log file in each
      // until we succeed or run out of options
      bool success = false;
      for (vector<string>::const_iterator dir = log_dirs.begin();
           dir != log_dirs.end();
           ++dir) {
        base_filename_ = *dir + "/" + stripped_filename;
        if ( CreateLogfile(time_pid_string) ) {
          success = true;
          break;
        }
      }
      // If we never succeeded, we have to give up
      if ( success == false ) {
        perror("Could not create logging file");
        fprintf(stderr, "COULD NOT CREATE A LOGGINGFILE %s!", time_pid_string);
        return;
      }
    }

    // Write a header message into the log file
    char file_header_string[512];  // Enough chars for time and binary info
    ostrstream file_header_stream(file_header_string,
                                  sizeof(file_header_string));
    file_header_stream.fill('0');
    file_header_stream << "Log file created at: "
                       << 1900+tm_time.tm_year << '/'
                       << setw(2) << 1+tm_time.tm_mon << '/'
                       << setw(2) << tm_time.tm_mday
                       << ' '
                       << setw(2) << tm_time.tm_hour << ':'
                       << setw(2) << tm_time.tm_min << ':'
                       << setw(2) << tm_time.tm_sec << '\n'
                       << "Running on machine: "
                       << LogDestination::hostname() << '\n'
                       << "Log line format: [IWEF]mmdd hh:mm:ss.uuuuuu "
                       << "threadid file:line] msg" << '\n'
                       << '\0';
    int header_len = strlen(file_header_string);
    fwrite(file_header_string, 1, header_len, file_);
    file_length_ += header_len;
    bytes_since_flush_ += header_len;
  }

  // Write to LOG file
  if ( !stop_writing ) {
    // fwrite() doesn't return an error when the disk is full, for
    // messages that are less than 4096 bytes. When the disk is full,
    // it returns the message length for messages that are less than
    // 4096 bytes. fwrite() returns 4096 for message lengths that are
    // greater than 4096, thereby indicating an error.
    errno = 0;
    fwrite(message, 1, message_len, file_);
    if ( FLAGS_stop_logging_if_full_disk &&
         errno == ENOSPC ) {  // disk full, stop writing to disk
      stop_writing = true;  // until the disk is
      return;
    } else {
      file_length_ += message_len;
      bytes_since_flush_ += message_len;
    }
  } else {
    if ( CycleClock_Now() >= next_flush_time_ )
      stop_writing = false;  // check to see if disk has free space.
    return;  // no need to flush
  }

  // See important msgs *now*.  Also, flush logs at least every 10^6 chars,
  // or every "FLAGS_logbufsecs" seconds.
  if ( force_flush ||
       (bytes_since_flush_ >= 1000000) ||
       (CycleClock_Now() >= next_flush_time_) ) {
    FlushUnlocked();
#ifdef OS_LINUX
    if (FLAGS_drop_log_memory) {
      if (file_length_ >= logging::kPageSize) {
        // don't evict the most recent page
        uint32 len = file_length_ & ~(logging::kPageSize - 1);
        posix_fadvise(fileno(file_), 0, len, POSIX_FADV_DONTNEED);
      }
    }
#endif
  }
}

