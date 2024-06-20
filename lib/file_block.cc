#include "file_block.h"

#include "file_system.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

void FileBlock::ClearMeta(FileBlockMeta *meta) {
  uint32_t index = meta->index_;
  uint32_t fh = meta->fh_;
  SPDLOG_INFO("clear file block index: {} fh: {}", index, fh);
  meta->used_ = false;
  meta->is_temp_ = false;
  meta->fh_ = -1;
  meta->file_cut_ = 0;
  meta->used_block_num_ = 0;
  // TODO
  ::memset(meta->block_id_, 0, sizeof(meta->block_id_));
}

bool FileBlock::WriteMeta(int32_t index) {
  uint64_t file_block_meta_size =
      FileSystem::Instance()->super_meta()->file_block_meta_size;
  uint64_t offset = file_block_meta_size * index;
  FileBlockMeta *meta = reinterpret_cast<FileBlockMeta *>(
      FileSystem::Instance()->file_block_handle()->base_addr() + offset);
  uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                     file_block_meta_size - sizeof(meta->crc_));
  meta->crc_ = crc;
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      meta, file_block_meta_size,
      FileSystem::Instance()->super_meta()->file_block_meta_offset_ + offset);
  if (ret != static_cast<int64_t>(file_block_meta_size)) [[unlikely]] {
    SPDLOG_ERROR("write file block meta index: {} error size: {} need: {}",
                 index, ret, file_block_meta_size);
    return false;
  }
  SPDLOG_INFO("write file block meta index: {} crc: {}", index, crc);
  return true;
}

void FileBlock::DumpMeta() {
  SPDLOG_INFO("dump file block meta:\n crc: {} used: {} fh: {} file_cut: {} used_block_num: {}",
              crc(), used(), fh(), file_cut(), used_block_num());
  for (uint32_t i = 0; i < meta_->used_block_num_; ++i) {
    SPDLOG_INFO("block index: {} block id: {}", i, get_block_id(i));
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
  SPDLOG_INFO("relase fh: {} file block index: {}", fh(), index_);
  FileBlock::ClearMeta(meta_);
  if (!WriteMeta()) {
    return false;
  }
  FileSystem::Instance()->file_block_handle()->PutFileBlockLock(index_);
  return true;
}
}  // namespace udisk::blockfs