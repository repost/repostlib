#ifndef RPDEBUG_H_
#define RPDEBUG_H_

/**
 * Alot of inspiration taken from glog
 */

#include <errno.h>
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#if REPOST_STRIP_LOG == 0
#define COMPACT_REPOST_LOG_INFO LogMessage( \
      __FILE__, __FUNCTION__,  __LINE__, INFO)
#else
#define COMPACT_REPOST_LOG_INFO NullStream()
#endif

#if REPOST_STRIP_LOG <= 1
#define COMPACT_REPOST_LOG_WARNING LogMessage( \
      __FILE__, __FUNCTION__, __LINE__, WARNING)
#else
#define COMPACT_REPOST_LOG_WARNING NullStream()
#endif

#if REPOST_STRIP_LOG <= 2
#define COMPACT_REPOST_LOG_ERROR LogMessage( \
      __FILE__, __FUNCTION__, __LINE__, ERROR)
#else
#endif

#if REPOST_STRIP_LOG <= 3
#define COMPACT_REPOST_LOG_FATAL LogMessage( \
      __FILE__, __FUNCTION__, __LINE__, FATAL)
#else
#define COMPACT_REPOST_LOG_FATAL NullStreamFatal()
#endif

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : LogMessageVoidify() & LOG(severity)

#define LOG(severity) COMPACT_REPOST_LOG_ ## severity.stream()

typedef int LogSeverity;

const int INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3, NUM_SEVERITIES = 4;

// DFATAL is FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
#define DFATAL_LEVEL ERROR
#else
#define DFATAL_LEVEL FATAL
#endif

extern const char* const LogSeverityNames[NUM_SEVERITIES];

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".

class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostringstream&) { }
};

class LogMessage {
public:
  class LogStream : public std::ostringstream {
  public:
    LogStream(int ctr)
      : std::ostringstream(std::ostringstream::out),
        ctr_(ctr) {
      self_ = this;
    }

    int ctr() const { return ctr_; }
    void set_ctr(int ctr) { ctr_ = ctr; }
    LogStream* self() const { return self_; }

  private:
    int ctr_;  // Counter hack (for the LOG_EVERY_X() macro)
    LogStream *self_;  // Consistency check hack
  };

public:
  // icc 8 requires this typedef to avoid an internal compiler error.
  typedef void (LogMessage::*SendMethod)();

  LogMessage(const char* file, const char* func, int line, LogSeverity severity);
  ~LogMessage();
  
  // Flush a buffered message to the sink set in the constructor.  Always
  // called by the destructor, it may also be called from elsewhere if
  // needed.  Only the first call is actioned; any later ones are ignored.
  void Flush();

  std::ostringstream& stream() { return *(data_->stream_); }

private:
  void Init(const char* file, const char* func, int line, LogSeverity severity);//,
            //void (LogMessage::*send_method)());

  void SendToLog();  // Actually dispatch to the logs

  // We keep the data in a separate struct so that each instance of
  // LogMessage uses less stack space.
  struct LogMessageData {
    LogMessageData() {};

    int preserved_errno_;      // preserved errno
    char* message_text_;  // Complete message text (points to selected buffer)
    LogStream* stream_alloc_;
    LogStream* stream_;
    char severity_;      // What level is this LogMessage logged at?
    int line_;                 // line number where logging call is.
    void (LogMessage::*send_method_)();  // Call this in destructor to send
    union {  // At most one of these is used: union to keep the size low.
      std::string* message_;             // NULL or string to write message into
    };
    time_t timestamp_;            // Time of creation of LogMessage
    struct ::tm tm_time_;         // Time of creation of LogMessage
    size_t num_prefix_chars_;     // # of chars of prefix in this message
    size_t num_chars_to_log_;     // # of chars of msg to send to log
    size_t num_chars_to_syslog_;  // # of chars of msg to send to syslog
    const char* basename_;        // basename of file that called LOG
    const char* fullname_;        // fullname of file that called LOG
    const char* func_;        // fullname of file that called LOG
    bool has_been_flushed_;       // false => data has not been flushed
    bool first_fatal_;            // true => this was first fatal msg

    ~LogMessageData();
   private:
    LogMessageData(const LogMessageData&);
    void operator=(const LogMessageData&);
  };

  LogMessageData* allocated_;
  LogMessageData* data_;

  LogMessage(const LogMessage&);
  void operator=(const LogMessage&);
};

class NullStream : public LogMessage::LogStream {
 public:
  // Initialize the LogStream so the messages can be written somewhere
  // (they'll never be actually displayed). This will be needed if a
  // NullStream& is implicitly converted to LogStream&, in which case
  // the overloaded NullStream::operator<< will not be invoked.
  NullStream() : LogMessage::LogStream(0) { }
  NullStream &stream() { return *this; }
 private:
};

// Do nothing. This operator is inline, allowing the message to be
// compiled away. The message will not be compiled away if we do
// something like (flag ? LOG(INFO) : LOG(ERROR)) << message; when
// SKIP_LOG=WARNING. In those cases, NullStream will be implicitly
// converted to LogStream and the message will be computed and then
// quietly discarded.
template<class T>
inline NullStream& operator<<(NullStream &str, const T &value) { return str; }



#endif
