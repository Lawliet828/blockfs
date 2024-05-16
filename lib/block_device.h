// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_DEVICE_H_
#define LIB_BLOCK_DEVICE_H_

#include "comm_utils.h"
#include "meta_defines.h"

namespace udisk {
namespace blockfs {

class BlockDevice {
 private:
  std::string dev_name_;
  int32_t dev_fd_direct_ = -1;
  int32_t dev_fd_cache_ = -1;
  uint64_t dev_size_ = 0;
  uint32_t sector_size_ = 0;
  bool verbose_discard_ = true;
  bool support_discard_ = false; /* trim supported */

 private:
  int64_t GetStringProperty(const char *property, char *val,
                            size_t maxlen) const;
  int64_t GetIntProperty(const char *property) const;
  bool SupportDiscard() const;
  bool IsRotational() const;
  int GetDev(char *dev, size_t max) const;
  int GetVendor(char *vendor, size_t max) const;
  int GetModel(char *model, size_t max) const;
  int GetSerial(char *serial, size_t max) const;
  int WholeDisk(char *device, size_t max) const;
  int WholeDisk(std::string *s) const;

 public:
  BlockDevice(bool io_no_merges = true);
  virtual ~BlockDevice();
  std::string dev_name() const { return dev_name_; }
  uint64_t dev_size() const { return dev_size_; }
  uint32_t block_size() const { return sector_size_; }

  const char *sysfsdir() const;
  bool IsBlkDev();
  dev_t BlkDevId() const;
  bool BlkDevGetSize();
  bool BlkDevGetSectorSize();
  bool BlkDevIsMisaligned();
  bool Open(const std::string &dev_name);
  int ReOpen(const std::string &dev_name);
  void Close();
  void Sync();
  int Fsync();
  int Fdatasync();
  int SyncFS();
  int Discard(uint64_t offset, uint64_t len);

  int64_t ReadCache(void *buf, uint64_t len);
  int64_t PreadCache(void *buf, uint64_t len, off_t offset);
  int64_t WriteCache(void *buf, uint64_t len);
  int64_t PwriteCache(void *buf, uint64_t len, off_t offset);

  int64_t ReadDirect(void *buf, uint64_t len);
  int64_t PreadDirect(void *buf, uint64_t len, off_t offset);
  int64_t WriteDirect(void *buf, uint64_t len);
  int64_t PwriteDirect(void *buf, uint64_t len, off_t offset);
};
}  // namespace blockfs
}  // namespace udisk
#endif