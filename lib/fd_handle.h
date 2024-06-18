#pragma once

#include <list>
#include <mutex>

#include "spdlog/spdlog.h"

namespace udisk::blockfs {

class FdHandle {
  FdHandle(const FdHandle &) = delete;
  FdHandle &operator=(const FdHandle &) = delete;

 private:
  size_t max_fd_num_ = 0;
  std::list<ino_t> free_fd_pool_;
  std::mutex mutex_;

 public:
  FdHandle() = default;
  explicit FdHandle(size_t max_fd_num) { set_max_fd_num(max_fd_num); }
  ~FdHandle() { free_fd_pool_.clear(); }

  void set_max_fd_num(size_t max_fd_num) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    SPDLOG_DEBUG("max fd num: {}", max_fd_num);
    for (ino_t fd = max_fd_num_; fd < max_fd_num; ++fd) {
      free_fd_pool_.push_back(fd);
    }
    max_fd_num_ = max_fd_num;
  }

  ino_t get_fd() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (free_fd_pool_.empty()) [[unlikely]] {
      SPDLOG_ERROR("fd pool is exhausted");
      errno = EMFILE;
      return -1;
    }
    SPDLOG_INFO("current fd pool size: {}", free_fd_pool_.size());
    ino_t fd = free_fd_pool_.front();
    free_fd_pool_.pop_front();
    SPDLOG_INFO("current fd: {} pool size: {}", fd, free_fd_pool_.size());
    return fd;
  }

  void put_fd(ino_t fd) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd > max_fd_num_) [[unlikely]] {
      SPDLOG_ERROR("fd large than max_fd_num: {}", max_fd_num_);
      return;
    }
    free_fd_pool_.push_back(fd);
  }
};
}
