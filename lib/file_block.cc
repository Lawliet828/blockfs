#include "file_store_udisk.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

void FileBlock::ClearMeta(FileBlockMeta *meta) {
  LOG(INFO) << "clear file block index: " << meta->index_
            << " fh: " << meta->fh_;
  meta->used_ = false;
  meta->is_temp_ = false;
  meta->seq_no_ = kReservedUnusedSeq;
  meta->fh_ = -1;
  meta->file_cut_ = 0;
  meta->used_block_num_ = 0;
  // TODO
  ::memset(meta->block_id_, 0, sizeof(meta->block_id_));
}

bool FileBlock::WriteMeta(int32_t index) {
  uint64_t offset =
      FileSystem::Instance()->super_meta()->file_block_meta_size_ * index;
  FileBlockMeta *meta = reinterpret_cast<FileBlockMeta *>(
      FileSystem::Instance()->file_block_handle()->base_addr() + offset);
  meta->crc_ =
      Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
            FileSystem::Instance()->super_meta()->file_block_meta_size_ -
                sizeof(meta->crc_));
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      meta, FileSystem::Instance()->super_meta()->file_block_meta_size_,
      FileSystem::Instance()->super_meta()->file_block_meta_offset_ + offset);
  if (unlikely(ret != static_cast<int64_t>(
              FileSystem::Instance()->super_meta()->file_block_meta_size_))) {
    LOG(ERROR) << "write file block meta index: " << index
               << " error size: " << ret << " need: "
               << FileSystem::Instance()->super_meta()->file_block_meta_size_;
    return false;
  }
  LOG(INFO) << "write file block meta index: " << index
            << " crc:" << meta->crc_;
  return true;
}

void FileBlock::DumpMeta() {
  LOG(INFO) << "dump file block meta:\n"
            << " crc: " << meta_->crc_ << "\n"
            << " used: " << meta_->used_ << "\n"
            << " seq_no: " << meta_->seq_no_ << "\n"
            << " fh: " << meta_->fh_ << "\n"
            << " file_cut: " << meta_->file_cut_ << "\n"
            << " padding: " << meta_->padding_ << "\n"
            << " used_block_num:" << meta_->used_block_num_;
  for (uint32_t i = 0; i < meta_->used_block_num_; ++i) {
    LOG(INFO) << "block index: " << i << " block id: " << meta_->block_id_[i];
  }
}

bool FileBlock::WriteMeta() { return FileBlock::WriteMeta(index_); }

bool FileBlock::ReleaseAll() {
  std::vector<uint32_t> block_list;
  for (uint32_t i = 0; i < meta_->used_block_num_; ++i) {
    block_list.push_back(meta_->block_id_[i]);
  }
  FileSystem::Instance()->block_handle()->PutFreeBlockIdLock(block_list);

  FileBlock::ClearMeta(meta_);
  if (!WriteMeta()) {
    return false;
  }
  FileSystem::Instance()->file_block_handle()->PutFileBlockLock(index_);
  return true;
}

bool FileBlock::ReleaseMyself() {
  LOG(INFO) << "relase fh: " << fh() << " file cut: " << file_cut()
            << " file block index: " << index_;
  FileBlock::ClearMeta(meta_);
  if (!WriteMeta()) {
    return false;
  }
  FileSystem::Instance()->file_block_handle()->PutFileBlockLock(index_);
  return true;
}
}  // namespace blockfs
}  // namespace udisk