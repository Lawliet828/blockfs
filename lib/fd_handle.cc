// Copyright (c) 2020 UCloud All rights reserved.
#include "file_store_udisk.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

bool FdHandle::InitializeMeta() {
  set_max_fd_num(FileStore::Instance()->super_meta()->max_file_num
                 << 1);
  return true;
}

void FdHandle::Dump() noexcept {}

void FdHandle::Dump(const std::string &file_name) noexcept {}

void FdHandle::set_max_fd_num(int32_t max_fd_num) noexcept {
  META_HANDLE_LOCK();
  LOG(DEBUG) << "max fd num: " << max_fd_num;
  for (int32_t index = max_fd_num_; index < max_fd_num; ++index) {
    free_fd_pool_.push_back(index);
  }
  max_fd_num_ = max_fd_num;
}

int32_t FdHandle::get_fd() noexcept {
  META_HANDLE_LOCK();
  if (free_fd_pool_.empty()) {
    LOG(ERROR) << "fd pool not enough, nothing left";
    errno = ENFILE;
    return -1;
  }
  LOG(INFO) << "current fd pool size: " << free_fd_pool_.size();
  int32_t fd = free_fd_pool_.front();
  free_fd_pool_.pop_front();
  LOG(INFO) << "current fd: " << fd << " pool size: " << free_fd_pool_.size();
  return fd;
}

void FdHandle::put_fd(int32_t fd) noexcept {
  META_HANDLE_LOCK();
  if (fd > max_fd_num_) {
    LOG(ERROR) << "fd large than max_fd_num: " << max_fd_num_;
    return;
  }
  free_fd_pool_.push_back(fd);
}
}  // namespace blockfs
}  // namespace udisk