
#include "rpversion.h"
#include "rpdebug.h"
#include <iomanip>
#include <vector>
#include <cstdio>
#include <fcntl.h>
#include <errno.h>

#ifdef OS_MACOSX
#define NAMED_SEMAPHORE
#include <time.h>
#endif

using namespace std;

static bool IsRepostLoggingRunning = false;
static LogSeverity  RepostLoggingLevel = DEBUG;
static vector<LogSink*> *RepostLogSinks = NULL;
static bool stop_writing = false;

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
        (*sink)->Send(data_->severity_, stream().str());
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

void LogStdoutSink::Send(LogSeverity severity, std::string msg)
{
    cout << msg << endl;
}

LogFileSink::LogFileSink(LogSeverity severity,
                             const char* base_filename)
   :base_filename_selected_(base_filename != NULL),
    base_filename_((base_filename != NULL) ? base_filename : ""),
    file_extension_("log"),
    file_(NULL),
    severity_(severity),
    bytes_since_flush_(0),
    file_length_(0),
    logbufsecs_(2),
    maxlogsize_(500e3),
    rollover_attempt_(kRolloverAttemptFrequency-1),
    next_flush_time_(0) 
{
    lock_ = new RpSemaphore(1);
}

LogFileSink::~LogFileSink() 
{
    lock_->Wait();
    if (file_ != NULL) 
    {
        fclose(file_);
        file_ = NULL;
    }
    delete(lock_);
}

int LogFileSink::LogSize() 
{
    ContextLock l(lock_);
    return file_length_;
}

void LogFileSink::SetBasename(const char* basename) 
{
    ContextLock l(lock_);
    base_filename_selected_ = true;
    if (base_filename_ != basename) 
    {
        /* Get rid of old log file since we are changing names */
        if (file_ != NULL) 
        {
            fclose(file_);
            file_ = NULL;
            rollover_attempt_ = kRolloverAttemptFrequency-1;
        }
        base_filename_ = basename;
    }
}

void LogFileSink::Flush() 
{
    ContextLock l(lock_);
    FlushUnlocked();
}

void LogFileSink::FlushUnlocked()
{
    if (file_ != NULL) 
    {
        fflush(file_);
        bytes_since_flush_ = 0;
    }
    /* Figure out when we are due for another flush. */
    ::time_t seconds = time(NULL);
    next_flush_time_ = seconds + logbufsecs_;
}

void LogFileSink::SetFlushTime(int secs)
{
    ContextLock l(lock_);
    logbufsecs_ = secs;
}

void LogFileSink::SetFileExtension(const char* extension)
{
    ContextLock l(lock_);
    file_extension_ = string(extension);
}

int LogFileSink::MaxLogSize()
{
    return maxlogsize_;
}

bool LogFileSink::CreateLogfile(std::string time_pid_string) 
{
    string string_filename = base_filename_+file_extension_+
        time_pid_string;
    const char* filename = string_filename.c_str();
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0664);
    if (fd == -1) return false;
    /* Mark the file close-on-exec. We don't really care if this fails */
    fcntl(fd, F_SETFD, FD_CLOEXEC);

    file_ = fdopen(fd, "a");  /* Make a FILE*. */
    if (file_ == NULL) {  /* Man, we're screwed! */
        close(fd);
        unlink(filename);  /* Erase the half-baked evidence: an unusable log file */
        return false;
    }
    return true;  // Everything worked
}

void LogFileSink::Write(bool force_flush, string msg)
{
    ContextLock l(lock_);
    ::time_t seconds = time(NULL);
    /* We don't log if the base_name_ is "" (which means "don't write") */
    if (base_filename_selected_ && base_filename_.empty()) 
    {
        return;
    }

    if (static_cast<int>(file_length_ >> 20) >= MaxLogSize()) 
    {
        if (file_ != NULL) fclose(file_);
        file_ = NULL;
        file_length_ = bytes_since_flush_ = 0;
        rollover_attempt_ = kRolloverAttemptFrequency-1;
    }

    /* If there's no destination file, make one before outputting */
    if (file_ == NULL) 
    {
        /*
        ** Try to rollover the log file every 32 log messages.  The only time
        ** this could matter would be when we have trouble creating the log
        ** file.  If that happens, we'll lose lots of log messages, of course!
        */
        if (++rollover_attempt_ != kRolloverAttemptFrequency) return;
        rollover_attempt_ = 0;

        struct ::tm tm_time;
        localtime_r(&seconds, &tm_time);

        /* The logfile's filename will have the date/time & pid in it */
        ostringstream time_pid_stream;
        time_pid_stream << 1900+tm_time.tm_year
            << setw(2) << 1+tm_time.tm_mon
            << setw(2) << tm_time.tm_mday
            << '-'
            << setw(2) << tm_time.tm_hour
            << setw(2) << tm_time.tm_min
            << setw(2) << tm_time.tm_sec
            << '.'
            << '\0';

        if (base_filename_selected_) 
        {
            if (!CreateLogfile(time_pid_stream.str())) 
            {
                fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", 
                            time_pid_stream.str().c_str());
                return;
            }
        } 

        /* Write a header message into the log file */
        ostringstream file_header_stream;
        file_header_stream << "Log file created at: "
            << 1900+tm_time.tm_year << '/'
            << setw(2) << 1+tm_time.tm_mon << '/'
            << setw(2) << tm_time.tm_mday
            << ' '
            << setw(2) << tm_time.tm_hour << ':'
            << setw(2) << tm_time.tm_min << ':'
            << setw(2) << tm_time.tm_sec << '\n'
            << '\0';
        int header_len = file_header_stream.str().length();
        const char* file_header_string = file_header_stream.str().c_str();
        fwrite(file_header_string, 1, header_len, file_);
        file_length_ += header_len;
        bytes_since_flush_ += header_len;
    }

    /* Write to LOG file */
    if ( !stop_writing ) 
    {
        /*
        ** fwrite() doesn't return an error when the disk is full, for
        ** messages that are less than 4096 bytes. When the disk is full,
        ** it returns the message length for messages that are less than
        ** 4096 bytes. fwrite() returns 4096 for message lengths that are
        ** greater than 4096, thereby indicating an error.
        */
        errno = 0;
        fwrite(msg.c_str(), 1, msg.length(), file_);
        if ( errno == ENOSPC ) 
        {  /* disk full, stop writing to disk */
            stop_writing = true;  /* until the disk is */
            return;
        } 
        else 
        {
            file_length_ += msg.length();
            bytes_since_flush_ += msg.length();
        }
    } 
    else 
    {
        if ( seconds >= next_flush_time_ )
            stop_writing = false;  /* check to see if disk has free space. */
        return;  /* no need to flush */
    }

    /*
    ** See important msgs *now*.  Also, flush logs at least every 10^6 chars,
    ** or every "FLAGS_logbufsecs" seconds.
    */
    if ( force_flush ||
            (bytes_since_flush_ >= 1000000) ||
            (seconds >= next_flush_time_) ) 
    {
        FlushUnlocked();
#ifdef OS_LINUX
        if (file_length_ >= getpagesize())
        {
            // don't evict the most recent page
            int len = file_length_ & ~(getpagesize() - 1);
            posix_fadvise(fileno(file_), 0, len, POSIX_FADV_DONTNEED);
        }
#endif
    }
}

