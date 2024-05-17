// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_H
#define LIB_BLOCK_H

#include <assert.h>

#include "block_device.h"

namespace udisk {
namespace blockfs {

class FileBlock : public std::enable_shared_from_this<FileBlock> {
 private:
  int32_t file_cut_;
  int32_t index_;
  FileBlockMeta *meta_;

 public:
  FileBlock() {}
  FileBlock(int32_t index, FileBlockMeta *meta) : index_(index), meta_(meta) {}
  ~FileBlock() {}

  bool ReleaseAll();
  bool ReleaseMyself();

  const int32_t index() const noexcept { return index_; }
  void set_index(int32_t index) noexcept { index_ = index; }
  FileBlockMeta *meta() const noexcept { return meta_; }

  void set_file_cut(uint32_t file_cut) noexcept { meta_->file_cut_ = file_cut; }
  uint32_t file_cut() const noexcept { return meta_->file_cut_; }

  void set_fh(int32_t fh) noexcept { meta_->fh_ = fh; }
  int32_t fh() const noexcept { return meta_->fh_; }

  void set_used(bool used) noexcept { meta_->used_ = used; }
  bool used() const noexcept { return meta_->used_; }

  const bool is_temp() const noexcept { return meta_->is_temp_; }
  void set_is_temp(bool is_temp) noexcept { meta_->is_temp_ = is_temp; }

  void set_used_block_num(uint32_t used_block_num) noexcept {
    meta_->used_block_num_ = used_block_num;
  }
  uint32_t used_block_num() const noexcept { return meta_->used_block_num_; }

  const uint32_t get_block_id(const uint32_t block_index) const noexcept {
    assert(block_index < kBlockFsFileBlockCapacity);
    return meta_->block_id_[block_index];
  }

  void add_block_id(uint32_t block_index, uint32_t block_id) {
    if (!is_block_full() && block_index < kBlockFsFileBlockCapacity) {
      meta_->block_id_[block_index] = block_id;
      ++meta_->used_block_num_;
    }
  }

  void add_block_id(uint32_t block_id) noexcept {
    if (!is_block_full()) {
      meta_->block_id_[meta_->used_block_num_] = block_id;
      ++meta_->used_block_num_;
    }
  }

  bool is_block_full() const noexcept {
    return (meta_->used_block_num_ == kBlockFsFileBlockCapacity);
  }

  void DumpMeta();
  bool WriteMeta();

  static bool WriteMeta(int32_t fb_index);
  static void ClearMeta(FileBlockMeta *meta);
};
}  // namespace blockfs
}  // namespace udisk
#endif