// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_META_HANDLE_H_
#define LIB_META_HANDLE_H_

#include <mutex>

#include "comm_utils.h"

namespace udisk {
namespace blockfs {

class MetaHandle : public NonCopyable {
 public:
  MetaHandle() = default;
  virtual ~MetaHandle() = default;

  // 设置和获取元数据起始地址
  virtual char *base_addr() noexcept { return base_addr_; }
  virtual void set_base_addr(char *addr) noexcept { base_addr_ = addr; }

  virtual bool InitializeMeta() { return true; };
  virtual bool FormatAllMeta() { return true; };
  virtual bool UpdateMeta() { return true; }

  virtual void Dump() noexcept {};
  virtual void Dump(const std::string &file_name) noexcept {};

 protected:
#define META_HANDLE_LOCK() std::lock_guard<std::mutex> lock(mutex_)
 protected:
  std::mutex mutex_;
  char *base_addr_;
};

}  // namespace blockfs
}  // namespace udisk
#endif  // LIB_META_HANDLE_H_