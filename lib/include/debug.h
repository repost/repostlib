#ifndef DEBUG_H_
#define DEBUG_H_

#include <errno.h>
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>

#if REPOST_STRIP_LOG == 0
#define COMPACT_REPOST_LOG_INFO repost::LogMessage( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_INFO(message) repost::LogMessage( \
      __FILE__, __LINE__, repost::INFO, message)
#else
#define COMPACT_REPOST_LOG_INFO repost::NullStream()
#define LOG_TO_STRING_INFO(message) repost::NullStream()
#endif

#if REPOST_STRIP_LOG <= 1
#define COMPACT_REPOST_LOG_WARNING repost::LogMessage( \
      __FILE__, __LINE__, repost::WARNING)
#define LOG_TO_STRING_WARNING(message) repost::LogMessage( \
      __FILE__, __LINE__, repost::WARNING, message)
#else
#define COMPACT_REPOST_LOG_WARNING repost::NullStream()
#define LOG_TO_STRING_WARNING(message) repost::NullStream()
#endif

#if REPOST_STRIP_LOG <= 2
#define COMPACT_REPOST_LOG_ERROR repost::LogMessage( \
      __FILE__, __LINE__, repost::ERROR)
#define LOG_TO_STRING_ERROR(message) repost::LogMessage( \
      __FILE__, __LINE__, repost::ERROR, message)
#else
#define COMPACT_REPOST_LOG_ERROR repost::NullStream()
#define LOG_TO_STRING_ERROR(message) repost::NullStream()
#endif

#if REPOST_STRIP_LOG <= 3
#define COMPACT_REPOST_LOG_FATAL repost::LogMessageFatal( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_FATAL(message) repost::LogMessage( \
      __FILE__, __LINE__, repost::FATAL, message)
#else
#define COMPACT_REPOST_LOG_FATAL repost::NullStreamFatal()
#define LOG_TO_STRING_FATAL(message) repost::NullStreamFatal()
#endif

#define LOG(severity) COMPACT_REPOST_LOG_ ## severity.stream()

class GOOGLE_GLOG_DLL_DECL LogMessage {
public:
  enum {
    // Passing kNoLogPrefix for the line number disables the
    // log-message prefix. Useful for using the LogMessage
    // infrastructure as a printing utility. See also the --log_prefix
    // flag for controlling the log-message prefix on an
    // application-wide basis.
    kNoLogPrefix = -1
  };

  // LogStream inherit from non-DLL-exported class (std::ostrstream)
  // and VC++ produces a warning for this situation.
  // However, MSDN says "C4275 can be ignored in Microsoft Visual C++
  // 2005 if you are deriving from a type in the Standard C++ Library"
  // http://msdn.microsoft.com/en-us/library/3tdb471s(VS.80).aspx
  // Let's just ignore the warning.
#ifdef _MSC_VER
# pragma warning(disable: 4275)
#endif
  class GOOGLE_GLOG_DLL_DECL LogStream : public std::ostrstream {
#ifdef _MSC_VER
# pragma warning(default: 4275)
#endif
  public:
    LogStream(char *buf, int len, int ctr)
      : ostrstream(buf, len),
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

  LogMessage(const char* file, int line, LogSeverity severity, int ctr,
             SendMethod send_method);

  // Two special constructors that generate reduced amounts of code at
  // LOG call sites for common cases.

  // Used for LOG(INFO): Implied are:
  // severity = INFO, ctr = 0, send_method = &LogMessage::SendToLog.
  //
  // Using this constructor instead of the more complex constructor above
  // saves 19 bytes per call site.
  LogMessage(const char* file, int line);

  // Used for LOG(severity) where severity != INFO.  Implied
  // are: ctr = 0, send_method = &LogMessage::SendToLog
  //
  // Using this constructor instead of the more complex constructor above
  // saves 17 bytes per call site.
  LogMessage(const char* file, int line, LogSeverity severity);

  // Constructor to log this message to a specified sink (if not NULL).
  // Implied are: ctr = 0, send_method = &LogMessage::SendToSinkAndLog if
  // also_send_to_log is true, send_method = &LogMessage::SendToSink otherwise.
  LogMessage(const char* file, int line, LogSeverity severity, LogSink* sink,
             bool also_send_to_log);

  // Constructor where we also give a vector<string> pointer
  // for storing the messages (if the pointer is not NULL).
  // Implied are: ctr = 0, send_method = &LogMessage::SaveOrSendToLog.
  LogMessage(const char* file, int line, LogSeverity severity,
             std::vector<std::string>* outvec);

  // Constructor where we also give a string pointer for storing the
  // message (if the pointer is not NULL).  Implied are: ctr = 0,
  // send_method = &LogMessage::WriteToStringAndLog.
  LogMessage(const char* file, int line, LogSeverity severity,
             std::string* message);

  // A special constructor used for check failures
  LogMessage(const char* file, int line, const CheckOpString& result);

  ~LogMessage();

  // Flush a buffered message to the sink set in the constructor.  Always
  // called by the destructor, it may also be called from elsewhere if
  // needed.  Only the first call is actioned; any later ones are ignored.
  void Flush();

  // An arbitrary limit on the length of a single log message.  This
  // is so that streaming can be done more efficiently.
  static const size_t kMaxLogMessageLen;

  // Theses should not be called directly outside of logging.*,
  // only passed as SendMethod arguments to other LogMessage methods:
  void SendToLog();  // Actually dispatch to the logs
  void SendToSyslogAndLog();  // Actually dispatch to syslog and the logs

  // Call abort() or similar to perform LOG(FATAL) crash.
  static void Fail() @ac_cv___attribute___noreturn@;

  std::ostream& stream() { return *(data_->stream_); }

  int preserved_errno() const { return data_->preserved_errno_; }

  // Must be called without the log_mutex held.  (L < log_mutex)
  static int64 num_messages(int severity);

private:
  // Fully internal SendMethod cases:
  void SendToSinkAndLog();  // Send to sink if provided and dispatch to the logs
  void SendToSink();  // Send to sink if provided, do nothing otherwise.

  // Write to string if provided and dispatch to the logs.
  void WriteToStringAndLog();

  void SaveOrSendToLog();  // Save to stringvec if provided, else to logs

  void Init(const char* file, int line, LogSeverity severity,
            void (LogMessage::*send_method)());

  // Used to fill in crash information during LOG(FATAL) failures.
  void RecordCrashReason(glog_internal_namespace_::CrashReason* reason);

  // Counts of messages sent at each priority:
  static int64 num_messages_[NUM_SEVERITIES];  // under log_mutex

  // We keep the data in a separate struct so that each instance of
  // LogMessage uses less stack space.
  struct GOOGLE_GLOG_DLL_DECL LogMessageData {
    LogMessageData() {};

    int preserved_errno_;      // preserved errno
    char* buf_;
    char* message_text_;  // Complete message text (points to selected buffer)
    LogStream* stream_alloc_;
    LogStream* stream_;
    char severity_;      // What level is this LogMessage logged at?
    int line_;                 // line number where logging call is.
    void (LogMessage::*send_method_)();  // Call this in destructor to send
    union {  // At most one of these is used: union to keep the size low.
      LogSink* sink_;             // NULL or sink to send message to
      std::vector<std::string>* outvec_; // NULL or vector to push message onto
      std::string* message_;             // NULL or string to write message into
    };
    time_t timestamp_;            // Time of creation of LogMessage
    struct ::tm tm_time_;         // Time of creation of LogMessage
    size_t num_prefix_chars_;     // # of chars of prefix in this message
    size_t num_chars_to_log_;     // # of chars of msg to send to log
    size_t num_chars_to_syslog_;  // # of chars of msg to send to syslog
    const char* basename_;        // basename of file that called LOG
    const char* fullname_;        // fullname of file that called LOG
    bool has_been_flushed_;       // false => data has not been flushed
    bool first_fatal_;            // true => this was first fatal msg

    ~LogMessageData();
   private:
    LogMessageData(const LogMessageData&);
    void operator=(const LogMessageData&);
  };

  static LogMessageData fatal_msg_data_exclusive_;
  static LogMessageData fatal_msg_data_shared_;

  LogMessageData* allocated_;
  LogMessageData* data_;

  friend class LogDestination;

  LogMessage(const LogMessage&);
  void operator=(const LogMessage&);
};

#endif
