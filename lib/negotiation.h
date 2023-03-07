
// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_NEGOTIATION_H_
#define LIB_NEGOTIATION_H_

#include "meta_defines.h"
#include "meta_handle.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Negotiation : public MetaHandle {
 public:
  Negotiation();
  ~Negotiation();

  NegotMeta *meta() { return reinterpret_cast<NegotMeta *>(base_addr()); }
  void set_crc(uint32_t crc) { meta()->crc_ = crc; }
  const uint32_t crc() noexcept { return meta()->crc_; }

  void set_resize_type(ResizeType type) { meta()->resize_type_ = type; }
  const ResizeType resize_type() noexcept { return meta()->resize_type_; }

  void set_resize_extend_num(uint32_t resize_extend_num) {
    meta()->resize_extend_num_ = resize_extend_num;
  }
  const uint32_t resize_extend_num() noexcept {
    return meta()->resize_extend_num_;
  }

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;
};
}  // namespace blockfs
}  // namespace udisk
#endif