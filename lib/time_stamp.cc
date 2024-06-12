#include <stdio.h>
#include <sys/time.h>

#include <chrono>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "time_stamp.h"

static_assert(sizeof(TimeStamp) == sizeof(int64_t),
              "TimeStamp is same size as int64_t");

TimeStamp TimeStamp::now() {
  // auto now = std::chrono::system_clock::now();
  // std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
