#pragma once

#include <list>
#include <mutex>

#include "spdlog/spdlog.h"

namespace udisk::blockfs {

class FdHandle {
  FdHandle(const FdHandle &) = delete;
  FdHandle &operator=(const FdHandle &) = delete;

 private:
  int max_fd_num_ = kMaxFileNum << 1;
  std::list<int> free_fd_pool_;
  std::mutex mutex_;

 public:
  FdHandle() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int fd = 0; fd < max_fd_num_; ++fd) {
      free_fd_pool_.push_back(fd);
    }
  }
  ~FdHandle() { free_fd_pool_.clear(); }

  int get_fd() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (free_fd_pool_.empty()) [[unlikely]] {
      SPDLOG_ERROR("fd pool is exhausted");
      errno = EMFILE;
      return -1;
    }
    SPDLOG_INFO("current fd pool size: {}", free_fd_pool_.size());
    int fd = free_fd_pool_.front();
    free_fd_pool_.pop_front();
    SPDLOG_INFO("current fd: {} pool size: {}", fd, free_fd_pool_.size());
    return fd;
  }

  void put_fd(int fd) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd > max_fd_num_) [[unlikely]] {
      SPDLOG_ERROR("fd large than max_fd_num: {}", max_fd_num_);
      return;
    }
    free_fd_pool_.push_back(fd);
  }
};
}
