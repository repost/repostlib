
#include "rpdebug.h"
#include <iomanip>

using namespace std;

const char*const LogSeverityNames[NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "FATAL"
};

LogMessage::LogMessage(const char* file, int line, LogSeverity severity) {
  Init(file, line, severity, &LogMessage::SendToLog);
}

void LogMessage::Init(const char* file,
                      int line,
                      LogSeverity severity,
                      void (LogMessage::*send_method)()) {

    allocated_ = new LogMessageData();
    data_ = allocated_;
    data_->stream_alloc_ = new LogStream(0);
    data_->stream_ = data_->stream_alloc_;
    data_->first_fatal_ = false;

    stream().fill('0');
    data_->preserved_errno_ = errno;
    data_->severity_ = severity;
    data_->line_ = line;
    data_->send_method_ = send_method;
    data_->timestamp_ = time( NULL );
    localtime_r(&data_->timestamp_, &data_->tm_time_);
//    int usecs = static_cast<int>((now - data_->timestamp_) * 1000000);

    data_->num_chars_to_log_ = 0;
    data_->num_chars_to_syslog_ = 0;
    //data_->basename_ = const_basename(file);
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

LogMessage::~LogMessage() {
  Flush();
  delete allocated_;
}

// Flush buffered message, called by the destructor, or any other function
// that needs to synchronize the log.
void LogMessage::Flush() {
  if (data_->has_been_flushed_ ) //|| data_->severity_ < FLAGS_minloglevel)
    return;

  //data_->num_chars_to_log_ = data_->stream_->pcount();
  data_->num_chars_to_syslog_ =
    data_->num_chars_to_log_ - data_->num_prefix_chars_;

  // Do we need to add a \n to the end of this message?
  bool append_newline =
      (data_->message_text_[data_->num_chars_to_log_-1] != '\n');
  char original_final_char = '\0';

  // If we do need to add a \n, we'll do it by violating the memory of the
  // ostrstream buffer.  This is quick, and we'll make sure to undo our
  // modification before anything else is done with the ostrstream.  It
  // would be preferable not to do things this way, but it seems to be
  // the best way to deal with this.
  if (append_newline) {
    original_final_char = data_->message_text_[data_->num_chars_to_log_];
    data_->message_text_[data_->num_chars_to_log_++] = '\n';
  }

  // Prevent any subtle race conditions by wrapping a mutex lock around
  // the actual logging action per se.
  {
    (this->*(data_->send_method_))();
  }

  if (append_newline) {
    // Fix the ostrstream back how it was before we screwed with it.
    // It's 99.44% certain that we don't need to worry about doing this.
    data_->message_text_[data_->num_chars_to_log_-1] = original_final_char;
  }

  // Note that this message is now safely logged.  If we're asked to flush
  // again, as a result of destruction, say, we'll do nothing on future calls.
  data_->has_been_flushed_ = true;
}
