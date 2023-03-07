#include "exponential_backoff.h"

#include "logging.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

template <typename Duration>
ExponentialBackoff<Duration>::ExponentialBackoff()
    : initialBackoff_(Duration(1000)),
      maxBackoff_(Duration(20000)),
      stepBackoff_(0),
      currentBackoff_(0) {}

template <typename Duration>
ExponentialBackoff<Duration>::ExponentialBackoff(Duration initialBackoff,
                                                 Duration maxBackoff,
                                                 bool isAbortAtMax)
    : initialBackoff_(initialBackoff),
      maxBackoff_(maxBackoff),
      stepBackoff_(0),
      currentBackoff_(0),
      isAbortAtMax_(isAbortAtMax) {
  if (initialBackoff < Duration(0)) {
    LOG(FATAL) << "Backoff must be positive value";
  }
  if (initialBackoff > maxBackoff) {
    LOG(FATAL) << "Max backoff must be greater than initial backoff.";
  }
}

template <typename Duration>
ExponentialBackoff<Duration>::ExponentialBackoff(Duration initialBackoff,
                                                 Duration maxBackoff,
                                                 Duration stepBackoff,
                                                 bool isAbortAtMax)
    : initialBackoff_(initialBackoff),
      maxBackoff_(maxBackoff),
      stepBackoff_(stepBackoff),
      currentBackoff_(0),
      isAbortAtMax_(isAbortAtMax) {
  if (initialBackoff < Duration(0)) {
    LOG(FATAL) << "Backoff must be positive value";
  }
  if (initialBackoff > maxBackoff) {
    LOG(FATAL) << "Max backoff must be greater than initial backoff.";
  }
}

template <typename Duration>
bool ExponentialBackoff<Duration>::CanTryNow() const {
  return GetTimeRemainingUntilRetry() == Duration(0);
}

template <typename Duration>
bool ExponentialBackoff<Duration>::CanDiscard() const {
  // Need to keep track of failures until maximum back-off period
  // has passed (since further failures can add to back-off).
  if (task_lifetime_ms_ == -1) {
    return false;
  }
  return AtMaxBackoff();
}

template <typename Duration>
void ExponentialBackoff<Duration>::ReportSuccess() {
  // Set error time to clock's epoch
  lastErrorTime_ = std::chrono::steady_clock::time_point();
  currentBackoff_ = Duration(0);
}

template <typename Duration>
void ExponentialBackoff<Duration>::ReportError() {
  lastErrorTime_ = std::chrono::steady_clock::now();
  if (currentBackoff_ >= maxBackoff_ && isAbortAtMax_) {
    LOG(ERROR) << "Max back-off reached, Abort! Abort!";
    ::abort();
  }
  if (currentBackoff_ == Duration(0)) {
    currentBackoff_ = initialBackoff_;
    LOG(DEBUG) << "init currentBackoff to initialBackoff";
  } else {
    // default exponential_backoff, otherwise use fixed step
    if (stepBackoff_ == Duration(0)) {
      currentBackoff_ = std::min(maxBackoff_, 2 * currentBackoff_);
    } else {
      int count = currentBackoff_.count() + kPerStepBackoff.count();
      currentBackoff_ = std::min(maxBackoff_, Duration(count));
    }
  }
}

template <typename Duration>
bool ExponentialBackoff<Duration>::AtMaxBackoff() const {
  return currentBackoff_ >= maxBackoff_;
}

template <typename Duration>
Duration ExponentialBackoff<Duration>::GetTimeRemainingUntilRetry() const {
  auto res = std::chrono::duration_cast<Duration>(
      (lastErrorTime_ + currentBackoff_) - std::chrono::steady_clock::now());
  return (res < Duration(0)) ? Duration(0) : res;
}

template <typename Duration>
int ExponentialBackoff<Duration>::GetTimeRemainingUntilRetrySecond() const {
  auto res = std::chrono::duration_cast<Duration>(
      (lastErrorTime_ + currentBackoff_) - std::chrono::steady_clock::now());
  auto time = (res < Duration(0)) ? Duration(0) : res;
  // Rounded up to int
  return (time.count() + jitter_factor_ms_) / Duration::period::den;
}

template <typename Duration>
std::chrono::steady_clock::time_point
ExponentialBackoff<Duration>::GetLastErrorTime() const {
  return lastErrorTime_;
}

template <typename Duration>
Duration ExponentialBackoff<Duration>::GetInitialBackoff() const {
  return initialBackoff_;
}

template <typename Duration>
Duration ExponentialBackoff<Duration>::GetMaxBackoff() const {
  return maxBackoff_;
}

// define template instance for some common usecases
template class ExponentialBackoff<std::chrono::microseconds>;
template class ExponentialBackoff<std::chrono::milliseconds>;
template class ExponentialBackoff<std::chrono::seconds>;

}  // namespace blockfs
}  // namespace udisk