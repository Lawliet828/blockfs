// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_SUPER_BLOCK_H_
#define LIB_SUPER_BLOCK_H_

#include "aligned_buffer.h"
#include "meta_defines.h"
#include "meta_handle.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class SuperBlock : public MetaHandle {
 private:
  AlignBufferPtr buffer_;

 public:
  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;

  SuperBlock();
  ~SuperBlock();
  SuperBlockMeta *meta() {
    return reinterpret_cast<SuperBlockMeta *>(base_addr());
  }
  void set_version(uint32_t ver) noexcept { meta()->version_ = ver; }
  void set_seq_no(seq_t seq) noexcept { meta()->seq_no_ = seq; }
  void set_curr_udisk_size(uint64_t curr_udisk_size) noexcept {
    meta()->curr_udisk_size_ = curr_udisk_size;
  }
  uint64_t get_curr_udisk_size() noexcept { return meta()->curr_udisk_size_; }

  bool set_uxdb_mount_point(const std::string &path);
  std::string uxdb_mount_point() noexcept { return meta()->uxdb_mount_point_; }
  bool is_mount_point(const std::string &path) noexcept;
  std::string uuid() noexcept { return meta()->uuid_; }
  uint64_t meta_size() noexcept { return meta()->block_data_start_offset_; }
  uint64_t curr_block_num() noexcept { return meta()->curr_block_num_; }

  bool CheckMountPoint(const std::string &path, bool isFile = false);
  bool WriteMeta();
};
}  // namespace blockfs
}  // namespace udisk
#endif