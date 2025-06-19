#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <sstream>
#include <string>
#include <memory>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define OS_WINDOWS
#elif defined(__CYGWIN__) || defined(__CYGWIN32__)
#define OS_CYGWIN
#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#define OS_MACOSX
#elif defined(__FreeBSD__)
#define OS_FREEBSD
#elif defined(__NetBSD__)
#define OS_NETBSD
#elif defined(__OpenBSD__)
#define OS_OPENBSD
#else
// TODO(hamaji): Add other platforms.
#endif

#if defined(OS_LINUX)
#include <unistd.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#endif

#if defined(_MSC_VER)
#define GSLAM_EXPORT __declspec(dllexport)
#elif defined(_MSC_VER) && 0
#define GSLAM_EXPORT __declspec(dllimport)
#else
#define GSLAM_EXPORT __attribute__((visibility("default")))
#endif

#ifdef ERROR
#undef ERROR
#endif
#ifdef LOG
#undef LOG
#endif

// Log severity level constants.
const int FATAL = -3;
const int ERROR = -2;
const int WARNING = -1;
const int INFO = 0;

// ------------------------- Glog compatibility ------------------------------

namespace infinite_sense {

typedef int LogSeverity;

const int GLOG_INFO = 0, GLOG_WARNING = 1, GLOG_ERROR = 2, GLOG_FATAL = 3, NUM_SEVERITIES = 4;

const char* const log_severity_names[NUM_SEVERITIES] = {"INFO", "WARNING", "ERROR", "FATAL"};

inline void GetTimeinfo(struct tm& ti) {
  time_t rawtime;
  time(&rawtime);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  // On Windows, use secure localtime_s not localtime.
  localtime_s(&ti, &rawtime);
#else
  // On non-Windows systems, use threadsafe localtime_r not localtime.
  localtime_r(&rawtime, &ti);
#endif
}

// FIXME: How to get tid on Windows?
inline int64_t GetTID() {
  // If gettid() could not be used, we use one of the following.
#if defined(OS_LINUX)
  return getpid();  // Linux:  getpid returns thread ID when gettid is absent
#elif defined(OS_WINDOWS)
  return GetCurrentThreadId();
#else
  return 0;
#endif
}

// Sink class used for integration with mock and test functions. If sinks are
// added, all log output is also sent to each sink through the send function.
// In this implementation, WaitTillSent() is called immediately after the send.
// This implementation is not thread safe.
class LogSink {
 public:
  virtual ~LogSink() = default;
  virtual void Send(LogSeverity severity, const char* full_filename, const char* base_filename, int line,
                    const struct tm* tm_time, const char* message, size_t message_len) = 0;
  virtual void WaitTillSent() = 0;

  static std::string ToString(LogSeverity severity, const char* file, int line, const struct ::tm* tm_time,
                              const char* message, size_t message_len) {
    std::ostringstream stream(std::string(message, message_len));
    stream.fill('0');

    // FIXME(jrvb): Updating this to use the correct value for usecs
    // requires changing the signature for both this method and
    // LogSink::send().  This change needs to be done in a separate CL
    // so subclasses of LogSink can be updated at the same time.
    int usecs = 0;

    stream << log_severity_names[severity + NUM_SEVERITIES - 1][0] << std::setw(2) << 1 + tm_time->tm_mon
           << std::setw(2) << tm_time->tm_mday << ' ' << std::setw(2) << tm_time->tm_hour << ':' << std::setw(2)
           << tm_time->tm_min << ':' << std::setw(2) << tm_time->tm_sec << '.' << std::setw(6) << usecs << ' '
           << std::setfill(' ') << std::setw(5) << GetTID() << std::setfill('0') << ' ' << file << ':' << line << "] ";

    stream << std::string(message, message_len);
    return stream.str();
  }
};

inline std::shared_ptr<std::set<LogSink*> >& GetLogSinksGlobal() {
  static std::shared_ptr<std::set<LogSink*> > sink(new std::set<LogSink*>());
  return sink;
}

// Note: the Log sink functions are not thread safe.
inline void AddLogSink(LogSink* sink) {
  // TODO(settinger): Add locks for thread safety.
  GetLogSinksGlobal()->insert(sink);
}
inline void RemoveLogSink(LogSink* sink) { GetLogSinksGlobal()->erase(sink); }

class LogFileSink : public LogSink {
 public:
  LogFileSink(LogSeverity severity, const char* base_filename) {
    m_severity = severity;
    m_base_file_name = base_filename;

    char fname[4096];
    struct tm ti{};
    GetTimeinfo(ti);
    sprintf(fname, "%s%04d%02d%02d-%02d-%02d-%02d.log", base_filename, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
            ti.tm_hour, ti.tm_min, ti.tm_sec);
    m_log_file_name = fname;

    m_log_file = fopen(fname, "wt+");
  }

  ~LogFileSink() override {
    if (m_log_file) {
      fclose(m_log_file);
      m_log_file = nullptr;
    }
  }

  void Send(LogSeverity severity, const char* full_filename, const char* base_filename, int line,
            const struct tm* tm_time, const char* message, size_t message_len) override {
    if (severity >= m_severity && m_log_file) {
      std::string ls = LogSink::ToString(severity, base_filename, line, tm_time, message, message_len);
      fwrite(ls.c_str(), ls.size(), 1, m_log_file);
    }
  }

  void WaitTillSent() override { fflush(m_log_file); }

 protected:
  LogSeverity m_severity;
  const char* m_base_file_name;
  std::string m_log_file_name;
  FILE* m_log_file;
};

typedef std::map<LogSeverity, LogFileSink*> LogSinkMap;

inline LogSinkMap& GetLogSinkMapGlobal() {
  static std::shared_ptr<LogSinkMap> log_sink_map(new LogSinkMap());
  return (*log_sink_map);
}

inline void SetLogDestination(LogSeverity severity, const char* base_filename) {
  LogSinkMap& g_log_sinks = GetLogSinkMapGlobal();

  auto it = g_log_sinks.find(severity);
  if (it == g_log_sinks.end()) {
    if (strlen(base_filename) == 0) return;

    // add a log file
    auto* log_file = new LogFileSink(severity, base_filename);
    g_log_sinks[severity] = log_file;
    AddLogSink(log_file);
  } else {
    LogFileSink* log_file = g_log_sinks[severity];

    // close old log file
    if (log_file) {
      RemoveLogSink(log_file);
      g_log_sinks.erase(it);
      delete log_file;
    }

    // open new log file
    if (strlen(base_filename) > 0) {
      log_file = new LogFileSink(severity, base_filename);
      g_log_sinks[severity] = log_file;
      AddLogSink(log_file);
    }
  }
}

}  // namespace infinite_sense

// ---------------------------- Logger Class --------------------------------

// Class created for each use of the logging macros.
// The logger acts as a stream and routes the final stream contents to the
// Android logcat output at the proper filter level.  If ANDROID is not
// defined, output is directed to std::cerr.  This class should not
// be directly instantiated in code, rather it should be invoked through the
// use of the log macros LG, LOG, or VLOG.
class MessageLogger {
 public:
  MessageLogger(const char* file, int line, const char* tag, int severity)
      : file_(file), line_(line), tag_(tag), severity_(severity) {
    StripBasename(std::string(file), &filename_only_);
  }

  // Output the contents of the stream to the proper channel on destruction.
  ~MessageLogger() {
    stream_ << "\n";

    std::cerr << filename_only_ << ":" << line_ << " " << stream_.str();

    LogToSinks(severity_);
    WaitForSinks();

    // Android logging at level FATAL does not terminate execution, so abort()
    // is still required to stop the program.
    if (severity_ == FATAL) {
      abort();
    }
  }

  // Return the stream associated with the logger object.
  std::stringstream& Stream() { return stream_; }

 private:
  void LogToSinks(int severity) {
    struct tm timeinfo{};
    infinite_sense::GetTimeinfo(timeinfo);

    std::set<infinite_sense::LogSink*>::iterator iter;
    // Send the log message to all sinks.
    for (iter = infinite_sense::GetLogSinksGlobal()->begin(); iter != infinite_sense::GetLogSinksGlobal()->end();
         ++iter) {
      (*iter)->Send(severity, file_.c_str(), filename_only_.c_str(), line_, &timeinfo, stream_.str().c_str(),
                    stream_.str().size());
    }
  }

  static void WaitForSinks() {
    // TODO(settinger): Add locks for thread safety.
    std::set<infinite_sense::LogSink*>::iterator iter;

    // Call WaitTillSent() for all sinks.
    for (iter = infinite_sense::GetLogSinksGlobal()->begin(); iter != infinite_sense::GetLogSinksGlobal()->end();
         ++iter) {
      (*iter)->WaitTillSent();
    }
  }

  static void StripBasename(const std::string& full_path, std::string* filename) {
    // TODO(settinger): Add support for OSs with different path separators.
    const char k_separator = '/';
    size_t pos = full_path.rfind(k_separator);
    if (pos != std::string::npos) {
      *filename = full_path.substr(pos + 1, std::string::npos);
    } else {
      *filename = full_path;
    }
  }

  std::string file_;
  std::string filename_only_;
  int line_;
  std::string tag_;
  std::stringstream stream_;
  int severity_;
};

// ---------------------- Logging Macro definitions --------------------------

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LoggerVoidify {
 public:
  LoggerVoidify() = default;
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(const std::ostream& s) {}
};

// Log only if condition is met.  Otherwise evaluates to void.
#define LOG_IF(severity, condition) \
  !(condition) ? (void)0 : LoggerVoidify() & MessageLogger((char*)__FILE__, __LINE__, "native", severity).Stream()

// Log only if condition is NOT met.  Otherwise evaluates to void.
#define LOG_IF_FALSE(severity, condition) LOG_IF(severity, !(condition))

// LG is a convenient shortcut for LOG(INFO). Its use is in new
// google3 code is discouraged and the following shortcut exists for
// backward compatibility with existing code.
#ifdef MAX_LOG_LEVEL
#define LOG(n) LOG_IF(n, n <= MAX_LOG_LEVEL)
#define VLOG(n) LOG_IF(n, n <= MAX_LOG_LEVEL)
#define LG LOG_IF(INFO, INFO <= MAX_LOG_LEVEL)
#define VLOG_IF(n, condition) LOG_IF(n, (n <= MAX_LOG_LEVEL) && condition)
#else
#define LOG(n) MessageLogger((char*)__FILE__, __LINE__, "native", n).Stream()   // NOLINT
#define VLOG(n) MessageLogger((char*)__FILE__, __LINE__, "native", n).Stream()  // NOLINT
#define LG MessageLogger((char*)__FILE__, __LINE__, "native", INFO).Stream()    // NOLINT
#define VLOG_IF(n, condition) LOG_IF(n, condition)
#endif

// Currently, VLOG is always on for levels below MAX_LOG_LEVEL.
#ifndef MAX_LOG_LEVEL
#define VLOG_IS_ON(x) (1)
#else
#define VLOG_IS_ON(x) (x <= MAX_LOG_LEVEL)
#endif

#ifndef NDEBUG
#define DLOG LOG
#else
#define DLOG(severity) \
  true ? (void)0 : LoggerVoidify() & MessageLogger((char*)__FILE__, __LINE__, "native", severity).stream()
#endif

// Log a message and terminate.
template <class T>
void LogMessageFatal(const char* file, int line, const T& message) {
  MessageLogger((char*)__FILE__, __LINE__, "native", FATAL).Stream() << message;
}

// ---------------------------- CHECK macros ---------------------------------

// Check for a given boolean condition.
#define CHECK(condition) LOG_IF_FALSE(FATAL, condition) << "Check failed: " #condition " "

#ifndef NDEBUG
// Debug only version of CHECK
#define DCHECK(condition) LOG_IF_FALSE(FATAL, condition) << "Check failed: " #condition " "
#else
// Optimized version - generates no code.
#define DCHECK(condition) \
  if (false) LOG_IF_FALSE(FATAL, condition) << "Check failed: " #condition " "
#endif  // NDEBUG

// ------------------------- CHECK_OP macros ---------------------------------

// Generic binary operator check macro. This should not be directly invoked,
// instead use the binary comparison macros defined below.
#define CHECK_OP(val1, val2, op) LOG_IF_FALSE(FATAL, ((val1)op(val2))) << "Check failed: " #val1 " " #op " " #val2 " "

// Check_op macro definitions
#define CHECK_EQ(val1, val2) CHECK_OP(val1, val2, ==)
#define CHECK_NE(val1, val2) CHECK_OP(val1, val2, !=)
#define CHECK_LE(val1, val2) CHECK_OP(val1, val2, <=)
#define CHECK_LT(val1, val2) CHECK_OP(val1, val2, <)
#define CHECK_GE(val1, val2) CHECK_OP(val1, val2, >=)
#define CHECK_GT(val1, val2) CHECK_OP(val1, val2, >)

#ifndef NDEBUG
// Debug only versions of CHECK_OP macros.
#define DCHECK_EQ(val1, val2) CHECK_OP(val1, val2, ==)
#define DCHECK_NE(val1, val2) CHECK_OP(val1, val2, !=)
#define DCHECK_LE(val1, val2) CHECK_OP(val1, val2, <=)
#define DCHECK_LT(val1, val2) CHECK_OP(val1, val2, <)
#define DCHECK_GE(val1, val2) CHECK_OP(val1, val2, >=)
#define DCHECK_GT(val1, val2) CHECK_OP(val1, val2, >)
#else
// These versions generate no code in optimized mode.
#define DCHECK_EQ(val1, val2) \
  if (false) CHECK_OP(val1, val2, ==)
#define DCHECK_NE(val1, val2) \
  if (false) CHECK_OP(val1, val2, !=)
#define DCHECK_LE(val1, val2) \
  if (false) CHECK_OP(val1, val2, <=)
#define DCHECK_LT(val1, val2) \
  if (false) CHECK_OP(val1, val2, <)
#define DCHECK_GE(val1, val2) \
  if (false) CHECK_OP(val1, val2, >=)
#define DCHECK_GT(val1, val2) \
  if (false) CHECK_OP(val1, val2, >)
#endif  // NDEBUG

// ---------------------------CHECK_NOTNULL macros ---------------------------

// Helpers for CHECK_NOTNULL(). Two are necessary to support both raw pointers
// and smart pointers.
template <typename T>
T& CheckNotNullCommon(const char* file, int line, const char* names, T& t) {
  if (t == NULL) {
    LogMessageFatal(file, line, std::string(names));
  }
  return t;
}

template <typename T>
T* CheckNotNull(const char* file, int line, const char* names, T* t) {
  return CheckNotNullCommon(file, line, names, t);
}

template <typename T>
T& CheckNotNull(const char* file, int line, const char* names, T& t) {
  return CheckNotNullCommon(file, line, names, t);
}

// Check that a pointer is not null.
#define CHECK_NOTNULL(val) CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

#ifndef NDEBUG
// Debug only version of CHECK_NOTNULL
#define DCHECK_NOTNULL(val) CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
#else
// Optimized version - generates no code.
#define DCHECK_NOTNULL(val) \
  if (false) CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
#endif  // NDEBUG _