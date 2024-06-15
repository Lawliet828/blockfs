#pragma once

#include "aligned_buffer.h"
#include "meta_defines.h"
#include "meta_handle.h"

namespace udisk::blockfs {

class SuperBlock : public MetaHandle {
 private:
  AlignBufferPtr buffer_;

 public:
  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;

  SuperBlock() = default;
  ~SuperBlock() { buffer_.reset(); }
  SuperBlockMeta *meta() {
    return reinterpret_cast<SuperBlockMeta *>(base_addr());
  }

  bool set_uxdb_mount_point(const std::string &path);
  std::string uxdb_mount_point() noexcept { return meta()->uxdb_mount_point_; }
  bool is_mount_point(const std::string &path) noexcept;
  std::string uuid() noexcept { return meta()->uuid_; }
  uint64_t meta_size() noexcept { return meta()->block_data_start_offset_; }

  bool WriteMeta();
};
}