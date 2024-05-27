// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_META_SHM_H
#define LIB_META_SHM_H

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <string>

#include "aligned_buffer.h"
#include "meta_defines.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

enum MemType {
  MEM_TYPE_SHAREDMEM = 0,
  MEM_TYPE_MEMALIGN = 1,
};
class ShmManager {
 public:
  ShmManager();
  ~ShmManager();

  std::string uuid() const { return super_.uuid_; }
  bool Initialize(bool create = true);
  bool Destroy() __attribute__((unused));
  static void CleanupDirtyShareMemory();
  static bool PrefetchSuperMeta(SuperBlockMeta &meta);

 private:
  static void CleanupShareMemory(const std::string &uuid);
  bool PrefetchSuperMeta();
  bool NewPosixAlignMem();
  bool ShmOpen();
  bool MemMap();
  bool MemUnMap();
  bool ReadAllMeta();
  void RegistMetaBaseAddr();

 private:
  MemType type_ = MEM_TYPE_MEMALIGN;
  AlignBufferPtr buffer_;

  int shm_fd_ = -1;
  char *shm_addr_;

  std::string shm_name_;
  int64_t shm_size_ = 0;

  SuperBlockMeta super_;
};

}  // namespace blockfs
}  // namespace udisk
#endif