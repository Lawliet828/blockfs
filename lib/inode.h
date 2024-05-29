#ifndef LIB_INODE_H_
#define LIB_INODE_H_

#include <fcntl.h>
#include <stdint.h>
#include <time.h>

#include <atomic>
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
  Inode() {}
  Inode(MetaName *meta) : meta_(meta) {}
  virtual ~Inode() {}

  void set_meta(MetaName *meta) noexcept { meta_ = meta; }
  MetaName *meta() noexcept { return meta_; }
  MetaItemNamePtr GetMetaItem(int32_t index) {
    auto it = item_maps_.find(index);
    if (it != item_maps_.end()) {
      return it->second;
    }
    return nullptr;
  }

  virtual void set_atime(time_t time) noexcept {};
  virtual time_t atime() noexcept { return 0; };
  virtual void set_mtime(time_t time) noexcept {};
  virtual time_t mtime() noexcept { return 0; };
  virtual void set_ctime(time_t time) noexcept {};
  virtual time_t ctime() noexcept { return 0; };

  virtual void stat(struct stat *buf) { return; }
  virtual int rename(const std::string &to) { return 0; }

  virtual bool UpdateMeta() { return false; }
  virtual bool WriteMeta() { return true; }
  virtual void DumpMeta() {}
};
}
#endif