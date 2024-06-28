#ifndef LIB_META_HANDLE_H_
#define LIB_META_HANDLE_H_

#include <mutex>

namespace udisk::blockfs {

class MetaHandle {
 private:
  MetaHandle(const MetaHandle &) = delete;
  MetaHandle &operator=(const MetaHandle &) = delete;

 public:
  MetaHandle() = default;
  virtual ~MetaHandle() = default;

  // 设置和获取元数据起始地址
  char *base_addr() const noexcept { return base_addr_; }
  void set_base_addr(char *addr) noexcept { base_addr_ = addr; }

  virtual bool InitializeMeta() = 0;
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

}
#endif  // LIB_META_HANDLE_H_