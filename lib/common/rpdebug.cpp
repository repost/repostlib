
#include "rpversion.h"
#include "rpdebug.h"
#include <iomanip>

using namespace std;

static bool IsRepostLoggingRunning = false;

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
    if (data_->has_been_flushed_ ) 
        return;
    cout << stream().str() << endl;
    data_->has_been_flushed_ = true;
}

void InitRepostLogging(void)
{
    if(IsRepostLoggingRunning == false)
    {
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
    }
}


