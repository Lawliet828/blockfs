#ifndef LIB_LOGGER_H_
#define LIB_LOGGER_H_

#include <assert.h>

#include <string>

#include "time_stamp.h"

namespace udisk::blockfs {

typedef enum {
  TRACE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
  NUM_LOG_LEVELS
} LogLevel;

class Logger {
 public:
  static LogLevel LogLevelConvert(const std::string& pLevel);
  static std::string LogLevelConvert(LogLevel eLevel);

 public:
  explicit Logger(LogLevel logLevel, const char* FILE, int LINE);

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  virtual ~Logger();

 public:
  static void set_min_level(LogLevel level);
  static LogLevel get_min_level();

  std::string GetLogName() const;

  std::string GetSavePath() const;

  void SetLogName(const std::string& logName);

  void SetSavePath(const std::string& savePath);

  std::string GetCurrentTime(int flag) const;

  void Append(const char* buf, unsigned int len);
  const char* GetBuffer() const;
  void ResetBuffer();
  void WriteLog();

 public:
  //这里是仿照陈硕的logSteam写的，但是我并没有像他那样重载那么多的<<操作符
  Logger& operator<<(bool v);
  Logger& operator<<(int8_t v);
  Logger& operator<<(uint8_t v);
  Logger& operator<<(int16_t v);
  Logger& operator<<(uint16_t v);
  Logger& operator<<(int v);
  Logger& operator<<(uint32_t v);
  Logger& operator<<(int64_t v);
  Logger& operator<<(uint64_t v);
  Logger& operator<<(float v);
  Logger& operator<<(double v);
  Logger& operator<<(const char* v);
  Logger& operator<<(const void* p);
  Logger& operator<<(const std::string& v);

 private:
  std::string logName_;   //日志名称
  std::string savePath_;  //日志保存目录
  LogLevel logLevel_;     //日志类型
  char buffer_[4096];

  static LogLevel min_log_level_;
};

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(level) Logger(level, __FILE__, __LINE__)

}
#endif
