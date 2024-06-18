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

  std::string uuid() noexcept { return meta()->uuid_; }
  uint64_t meta_size() noexcept { return meta()->block_data_start_offset_; }

  bool WriteMeta();
};
}