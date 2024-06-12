// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_TIME_STAMP_H_
#define LIB_TIME_STAMP_H_

#include <stdint.h>

#include <string>

#include "comm_utils.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

///
/// Time stamp in UTC, in microseconds resolution.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
///
class TimeStamp {
 public:
  ///
  /// Constucts an invalid TimeStamp.
  ///
  TimeStamp() : microSecondsSinceEpoch_(0) {}

  ///
  /// Constucts a TimeStamp at specific time
  ///
  /// @param microSecondsSinceEpoch
  explicit TimeStamp(int64_t microSecondsSinceEpochArg)
      : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

  void swap(TimeStamp& that) {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  // default copy/assignment/dtor are Okay

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  // for internal usage.
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(microSecondsSinceEpoch_ /
                               kMicroSecondsPerSecond);
  }

  ///
  /// Get time of now.
  ///
  static TimeStamp now();

  static const int kMicroSecondsPerSecond = 1000 * 1000;

  static TimeStamp addTime(TimeStamp timestamp, double seconds) {
    int64_t delta =
        static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
    return TimeStamp(timestamp.microSecondsSinceEpoch() + delta);
  }

 private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(TimeStamp lhs, TimeStamp rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

}  // namespace blockfs
}  // namespace udisk
#endif