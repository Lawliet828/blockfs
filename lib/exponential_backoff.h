// Copyright (c) 2020 UCloud All rights reserved.
#ifndef UDISK_BACKOFF_POLICY_H_
#define UDISK_BACKOFF_POLICY_H_

#include <stdint.h>

#include <chrono>
#include <memory>

namespace udisk {
namespace blockfs {

// default pc task retry 20 times, max
const std::chrono::milliseconds kInitialBackoff{2000};
const std::chrono::milliseconds kMaximumBackoff{20000};
const std::chrono::milliseconds kPerStepBackoff{2000};

/**
 * Exponential backoff for generic keys using trial and error.
 *
 * This utility implements exponential backoff for keeping track of when to
 * retry something. A separate error count is maintained for each "key" used,
 * so that a different backoff can generated for each item.
 */
template <typename Duration>
class ExponentialBackoff {
 public:
  /**
   * Make ExponentialBackoff default constructible. Though it is not very
   * much usable at all unless reassigned with valid one.
   */
  ExponentialBackoff();

  explicit ExponentialBackoff(Duration initialBackoff, Duration maxBackoff,
                              bool isAbortAtMax = false);

  /**
   * @param initialBackoff  The length of time to wait before retrying
   *                        after the first error.
   * @param maxBackoff      The maximum backoff period to use.
   * @param isAbortAtMax    Abort at max backoff
   */
  explicit ExponentialBackoff(Duration initialBackoff, Duration maxBackoff,
                              Duration stepBackoff, bool isAbortAtMax = false);

  /**
   * Should we wait or not?
   *
   * If there was an error, we want to wait for the backoff period specified
   * in the constructor. If there is still an error then keep doubling the
   * backoff time, up to the specified maximum backoff.
   */
  bool CanTryNow() const;

  // Returns true if a request for the resource this item tracks should
  // be rejected at the present time due to exponential back-off policy.
  bool CanDiscard() const;

  /**
   * Have we reached the maximum backoff?
   */
  bool AtMaxBackoff() const;

  /**
   * Clear backoff period
   */
  void ReportSuccess();

  /**
   * Note that we should back off more
   */
  void ReportError();

  // Inform this item that a request for the network resource it is
  // tracking was made, and whether it failed or succeeded.
  void ReportStatus(bool status) {
    if (status) {
      ReportSuccess();
    } else {
      ReportError();
    }
  }

  /**
   * Get the time remaining until next retry
   */
  Duration GetTimeRemainingUntilRetry() const;

  int GetTimeRemainingUntilRetrySecond() const;

  /**
   * Get latest error timestamp
   */
  std::chrono::steady_clock::time_point GetLastErrorTime() const;

  /**
   * Getters for backoff time
   */
  Duration GetInitialBackoff() const;
  Duration GetMaxBackoff() const;

 private:
  /**
   * 退避算法有多种表现形式,主要功能是发生某类事件时,
   * 为了避免频繁地触发某个行为,而采取延长行为反应时间的措施
   * 指数退避算法需要五个参数:
   * 初始等待周期(Initial wait period)
   * 再次等待周期(Secondary wait period)
   * 增长因子(Increase factor)
   * 最长间隔时间(Maximum interval)
   * 稳定周期因子(Stable period factor)
   */
  Duration initialBackoff_;
  Duration maxBackoff_;
  Duration stepBackoff_;

  // Current backoff. If things are good then it is Duration(0)
  Duration currentBackoff_;

  bool isAbortAtMax_ = false;

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  int task_lifetime_ms_ = 10 * 1000;  // default 10s

  // allow 10ms thread dispatching cost
  double jitter_factor_ms_ = 5;

  // Time point of last error
  std::chrono::steady_clock::time_point lastErrorTime_;
};
}  // namespace blockfs
}  // namespace udisk

#endif /* UDISK_BACKOFF_POLICY_H_ */
