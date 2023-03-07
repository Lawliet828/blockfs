// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FD_HANDLE_H_
#define LIB_FD_HANDLE_H_

#include <list>
#include <mutex>

#include "meta_handle.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class FdHandle : public MetaHandle {
 private:
  int32_t max_fd_num_ = 0;
  std::list<int32_t> free_fd_pool_;

 public:
  FdHandle() = default;
  explicit FdHandle(int32_t max_fd_num) { set_max_fd_num(max_fd_num); }
  ~FdHandle() { free_fd_pool_.clear(); }
  void set_max_fd_num(int32_t max_fd_num) noexcept;
  int32_t get_fd() noexcept;
  void put_fd(int32_t fd) noexcept;

  virtual bool InitializeMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;
};
}  // namespace blockfs
}  // namespace udisk
#endif