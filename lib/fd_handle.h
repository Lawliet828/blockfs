#pragma once

#include <list>
#include <mutex>

#include "spdlog/spdlog.h"

namespace udisk::blockfs {

class FdHandle {
  FdHandle(const FdHandle &) = delete;
  FdHandle &operator=(const FdHandle &) = delete;

 private:
  int32_t max_fd_num_ = 0;
  std::list<int32_t> free_fd_pool_;
  std::mutex mutex_;

 public:
  FdHandle() = default;
  explicit FdHandle(int32_t max_fd_num) { set_max_fd_num(max_fd_num); }
  ~FdHandle() { free_fd_pool_.clear(); }

  void set_max_fd_num(int32_t max_fd_num) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    SPDLOG_DEBUG("max fd num: {}", max_fd_num);
    for (int32_t index = max_fd_num_; index < max_fd_num; ++index) {
      free_fd_pool_.push_back(index);
    }
    max_fd_num_ = max_fd_num;
  }

  int32_t get_fd() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (free_fd_pool_.empty()) [[unlikely]] {
      SPDLOG_ERROR("fd pool is exhausted");
      errno = ENFILE;
      return -1;
    }
    SPDLOG_INFO("current fd pool size: {}", free_fd_pool_.size());
    int32_t fd = free_fd_pool_.front();
    free_fd_pool_.pop_front();
    SPDLOG_INFO("current fd: {} pool size: {}", fd, free_fd_pool_.size());
    return fd;
  }

  void put_fd(int32_t fd) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd > max_fd_num_) [[unlikely]] {
      SPDLOG_ERROR("fd large than max_fd_num: {}", max_fd_num_);
      return;
    }
    free_fd_pool_.push_back(fd);
  }
};
}
