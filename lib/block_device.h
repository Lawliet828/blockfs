#ifndef LIB_BLOCK_DEVICE_H_
#define LIB_BLOCK_DEVICE_H_

#include "comm_utils.h"
#include "meta_defines.h"

namespace udisk::blockfs {

class BlockDevice {
 private:
  std::string dev_name_;
  int32_t dev_fd_direct_ = -1;
  int32_t dev_fd_cache_ = -1;
  uint64_t dev_size_ = 0;
  uint32_t sector_size_ = 0;

 public:
  BlockDevice() {}
  virtual ~BlockDevice();
  std::string dev_name() const { return dev_name_; }
  uint64_t dev_size() const { return dev_size_; }
  uint32_t block_size() const { return sector_size_; }

  bool IsBlkDev();
  bool BlkDevGetSize();
  bool BlkDevGetSectorSize();
  bool Open(const std::string &dev_name);
  void Close();
  int Fsync();

  int64_t PreadCache(void *buf, uint64_t len, off_t offset);
  int64_t PwriteCache(void *buf, uint64_t len, off_t offset);

  int64_t PreadDirect(void *buf, uint64_t len, off_t offset);
  int64_t PwriteDirect(void *buf, uint64_t len, off_t offset);
};
}
#endif