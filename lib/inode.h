#pragma once

#include <fcntl.h>
#include <stdint.h>
#include <time.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace udisk::blockfs {

template <typename MetaName, typename MetaItemName>
class Inode {
 public:
  typedef std::shared_ptr<MetaItemName> MetaItemNamePtr;

 protected:
  MetaName *meta_;
  std::mutex mutex_;
  std::unordered_map<int32_t, MetaItemNamePtr> item_maps_;
  bool deleted_;

 public:
  Inode() = default;
  Inode(MetaName *meta) : meta_(meta) {}
  virtual ~Inode() = default;

  void set_meta(MetaName *meta) noexcept { meta_ = meta; }
  MetaName *meta() noexcept { return meta_; }
  MetaItemNamePtr GetMetaItem(int32_t index) {
    auto it = item_maps_.find(index);
    if (it != item_maps_.end()) {
      return it->second;
    }
    return nullptr;
  }

  virtual void set_atime(time_t time) noexcept = 0;
  virtual time_t atime() noexcept = 0;
  virtual void set_mtime(time_t time) noexcept = 0;
  virtual time_t mtime() noexcept = 0;
  virtual void set_ctime(time_t time) noexcept = 0;
  virtual time_t ctime() noexcept = 0;

  virtual void stat(struct stat *buf) = 0;
  virtual int rename(const std::string &to) { return 0; }

  virtual bool UpdateMeta() { return false; }
  virtual bool WriteMeta() { return true; }
  virtual void DumpMeta() {}
};
}