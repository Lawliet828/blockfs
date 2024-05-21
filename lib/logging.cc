#include "logging.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>

#include "comm_utils.h"
#include "current_thread.h"

namespace udisk {
namespace blockfs {

__thread char t_time[64];
__thread time_t t_lastSecond;

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

size_t ConvertHex(char buf[], uintptr_t value) {
  uintptr_t i = value;
  char* p = buf;

  do {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

typedef struct {
  LogLevel level_;
  std::string name_;
} LevelName;

static LevelName kLevelTable[] = {{TRACE, "TRACE"},
                                  {DEBUG, "DEBUG"},
                                  {INFO, "INFO"},
                                  {WARNING, "WARN"},
                                  {ERROR, "ERROR"},
                                  {FATAL, "FATAL"},
                                  {NUM_LOG_LEVELS, "NUM_LOG_LEVELS"}};

LogLevel Logger::LogLevelConvert(const std::string& pLevel) {
  for (uint32_t i = 0; i < ARRAY_SIZE(kLevelTable); ++i) {
    if (kLevelTable[i].name_ == pLevel) {
      return kLevelTable[i].level_;
    }
  }
  return INFO;
}

std::string Logger::LogLevelConvert(LogLevel level_) {
  for (unsigned i = 0; i < ARRAY_SIZE(kLevelTable); ++i) {
    if (kLevelTable[i].level_ == level_) {
      return kLevelTable[i].name_;
    }
  }
  return "INFO";
}

void Logger::set_min_level(LogLevel level) { min_log_level_ = level; }

LogLevel Logger::get_min_level() { return min_log_level_; }

LogLevel Logger::min_log_level_ = INFO;

Logger::Logger(LogLevel logLevel, const char* FILE, int LINE)
    : logLevel_(logLevel), time_(TimeStamp::now()) {
  if (logLevel_ >= min_log_level_) {
    ::memset(buffer_, 0, 4096);
    logName_ += GetCurrentTime(0);
    std::string level = LogLevelConvert(logLevel);
    std::string now = GetCurrentTime(1);
    CurrentThread::tid();
    snprintf(buffer_, sizeof(buffer_) - 1, "%s%s %s %s:%d - ",
             CurrentThread::tidString(), now.c_str(), level.c_str(), FILE,
             LINE);
  }
}

Logger::~Logger() {
  if (logLevel_ >= min_log_level_) {
    std::cerr << buffer_ << std::endl << std::flush;
    WriteLog();
  }
}

void Logger::Dump() const {
  printf("\n=====Logger Dump START ========== \n");
  printf("logName_=%s ", logName_.c_str());
  printf("savePath_=%s ", savePath_.c_str());
  printf("buffer_=%s ", buffer_);
  printf("\n===Logger DUMP END ============\n");
}

std::string Logger::GetLogName() const { return logName_; }

std::string Logger::GetSavePath() const { return savePath_; }

void Logger::SetLogName(const std::string& logName) { logName_ = logName; }

void Logger::SetSavePath(const std::string& savePath) { savePath_ = savePath; }

std::string Logger::GetCurrentTime(int flag) const {
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch /
                                       TimeStamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch %
                                      TimeStamp::kMicroSecondsPerSecond);
  int milliseconds = microseconds / 1000;
  if (seconds != t_lastSecond) {
    t_lastSecond = seconds;
    struct tm tm_time;
    ::memset(&tm_time, 0, sizeof(struct tm));

    ::localtime_r(&seconds, &tm_time);

    int len =
        snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    (void)len;
  }

  char time_str[64] = {0};
  ::snprintf(time_str, sizeof(time_str) - 1, "%s.%03d", t_time, milliseconds);
  return time_str;
}

void Logger::Append(const char* buf, unsigned int len) {
  if (::strlen(buffer_) + len <= sizeof(buffer_)) {
    ::memcpy(buffer_ + ::strlen(buffer_), buf, len);
  } else {
    // TODO cerr<< "no enough space in buffer"
  }
}

const char* Logger::GetBuffer() const { return buffer_; }

void Logger::ResetBuffer() { ::memset(buffer_, 0, sizeof(buffer_)); }

Logger& Logger::operator<<(bool v) {
  char temp[8];
  sprintf(temp, "%s", v ? "true" : "false");
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(int8_t v) {
  char temp[8];
  sprintf(temp, "%d", v);
  *this << temp;
  return *this;
}
Logger& Logger::operator<<(uint8_t v) {
  char temp[8];
  sprintf(temp, "%d", v);
  *this << temp;
  return *this;
}
Logger& Logger::operator<<(int16_t v) {
  char temp[16];
  sprintf(temp, "%d", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(uint16_t v) {
  char temp[16];
  sprintf(temp, "%u", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(int v) {
  char temp[32];
  sprintf(temp, "%d", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(uint32_t v) {
  //由于是unsigned int，使用64的长度
  char temp[64];
  sprintf(temp, "%u", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(int64_t v) {
  //由于是unsigned int，使用64的长度
  char temp[64];
  sprintf(temp, "%ld", v);
  *this << temp;
  return *this;
}
Logger& Logger::operator<<(uint64_t v) {
  //由于是unsigned int，使用64的长度
  char temp[64];
  sprintf(temp, "%lu", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(float v) {
  *this << static_cast<double>(v);
  return *this;
}

Logger& Logger::operator<<(double v) {
  char temp[128];
  //这里默认仅仅保留三位小数
  sprintf(temp, "%.3lf", v);
  *this << temp;
  return *this;
}

Logger& Logger::operator<<(const char* v) {
  if (v) {
    Append(v, ::strlen(v));
  } else {
    Append("(null)", 6);
  }
  return *this;
}

Logger& Logger::operator<<(const void* p) {
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  if (p) {
    Append("0x", 2);
    char buffer[16] = {0};
    size_t len = ConvertHex(buffer, v);
    Append(buffer, len + 2);
  }
  return *this;
}

Logger& Logger::operator<<(const std::string& v) {
  Append(v.c_str(), v.size());
  return *this;
}

void Logger::WriteLog() {}

}  // namespace blockfs
}  // namespace udisk