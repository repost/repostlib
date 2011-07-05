
#include "rpdebug.h"
#include <iomanip>

using namespace std;

const char*const LogSeverityNames[NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "FATAL"
};

LogMessage::LogMessage(const char* file, int line, LogSeverity severity) 
{
    Init(file, line, severity);//, &LogMessage::SendToLog);
}

LogMessage::LogMessageData::~LogMessageData() {
  delete stream_alloc_;
}

void LogMessage::Init(const char* file,
                      int line,
                      LogSeverity severity)//,
                      //void (LogMessage::*send_method)()) 
{
    allocated_ = new LogMessageData();
    data_ = allocated_;
    data_->stream_alloc_ = new LogStream(0);
    data_->stream_ = data_->stream_alloc_;

    stream().fill('0');
    data_->severity_ = severity;
    data_->line_ = line;
 //   data_->send_method_ = send_method;
    data_->timestamp_ = time( NULL );
    localtime_r(&data_->timestamp_, &data_->tm_time_);
//    int usecs = static_cast<int>((now - data_->timestamp_) * 1000000);
    data_->fullname_ = file;
    data_->has_been_flushed_ = false;

    // If specified, prepend a prefix to each line.  For example:
    //    I1018 160715 f5d4fbb0 logging.cc:1153]
    //    (log level, GMT month, date, time, thread_id, file basename, line)
    // We exclude the thread_id for the default thread.
        stream() << LogSeverityNames[severity][0]
            << setw(2) << 1+data_->tm_time_.tm_mon
            << setw(2) << data_->tm_time_.tm_mday
            << ' '
            << setw(2) << data_->tm_time_.tm_hour  << ':'
            << setw(2) << data_->tm_time_.tm_min   << ':'
            << setw(2) << data_->tm_time_.tm_sec   << "."
            //<< setw(6) << usecs
            << ' '
            << setfill(' ') << setw(5)
            //<< static_cast<unsigned int>(GetTID()) << setfill('0')
            << ' '
            << data_->basename_ << ':' << data_->line_ << "] ";
    //data_->num_prefix_chars_ = data_->stream_->pcount();
}

LogMessage::~LogMessage() 
{
  Flush();
  delete allocated_;
}

// Flush buffered message, called by the destructor, or any other function
// that needs to synchronize the log.
void LogMessage::Flush() 
{
    if (data_->has_been_flushed_ ) //|| data_->severity_ < FLAGS_minloglevel)
        return;
    cout << stream();
    cout << "HELLOWORLD!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    data_->has_been_flushed_ = true;
}
