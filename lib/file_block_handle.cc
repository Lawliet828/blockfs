#include <assert.h>

#include "crc.h"
#include "file_system.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

bool FileBlockHandle::InitializeMeta() {
  LOG(DEBUG)
      << "total file block num: "
      << FileSystem::Instance()->super_meta()->max_file_block_num;
  FileBlockMeta *meta;
  for (uint32_t index = 0;
       index < FileSystem::Instance()->super_meta()->max_file_block_num;
       ++index) {
    meta = reinterpret_cast<FileBlockMeta *>(base_addr() +
        FileSystem::Instance()->super_meta()->file_block_meta_size_ * index);
    uint32_t crc =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              FileSystem::Instance()->super_meta()->file_block_meta_size_ -
                  sizeof(meta->crc_));
    if (meta->crc_ != crc) [[unlikely]] {
      LOG(ERROR) << "debug file block meta: \n"
                 << " file block index: " << index << "\n"
                 << " crc error\n"
                 << " read crc: " << meta->crc_ << "\n"
                 << " cal crc: " << crc << "\n"
                 << " used: " << meta->used_ << "\n"
                 << " fh: " << meta->fh_ << "\n"
                 << " used_block_num:" << meta->used_block_num_;
      for (uint32_t i = 0; i < meta->used_block_num_; ++i) {
        LOG(ERROR) << "block index: " << i
                   << " block id: " << meta->block_id_[i];
      }
      return false;
    }
    if (meta->used_) {
      // 临时文件直接清理掉, 前面还原文件的时候直接过滤掉
      if (meta->is_temp_) {
        LOG(WARNING) << "clear temp file block index: " << index
                     << " fh: " << meta->fh_;
        FileBlock::ClearMeta(meta);
        if (!FileBlock::WriteMeta(index)) {
          return false;
        }
        continue;
      }
      const FilePtr &file =
          FileSystem::Instance()->file_handle()->GetCreatedFileNoLock(meta->fh_);
      if (!file) [[unlikely]] {
        LOG(ERROR) << "file block meta: " << index
                   << " invalid for fh: " << meta->fh_
                   << " is_temp: " << meta->is_temp_
                   << " used_block_num: " << meta->used_block_num_;
        for (uint32_t i = 0; i < meta->used_block_num_; ++i) {
          // LOG(ERROR) << "block index: " << i
          //            << " block id: " << meta->block_id_[i];
        }
        return false;
      }
      FileBlockPtr fb = std::make_shared<FileBlock>(index, meta);
      // 把Block从空闲列表中摘出来, 不能被分配出去
      for (uint32_t i = 0; i < fb->used_block_num(); ++i) {
        LOG(DEBUG) << file->file_name() << " block index: " << i
                   << " block id: " << fb->get_block_id(i);
        if (!FileSystem::Instance()->block_handle()->GetSpecificBlockId(
                fb->get_block_id(i))) {
          return false;
        }
      }
      file->AddFileBlockNoLock(fb);
    } else {
      free_fbhs_.push_back(index);
    }
  }
  return true;
}

bool FileBlockHandle::FormatAllMeta() {
  uint64_t file_block_meta_total_size =
      FileSystem::Instance()->super_meta()->file_block_meta_total_size_;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      file_block_meta_total_size, FileSystem::Instance()->dev()->block_size());

  FileBlockMeta *meta;
  for (uint32_t i = 0;
       i < FileSystem::Instance()->super_meta()->max_file_block_num;
       ++i) {
    meta = reinterpret_cast<FileBlockMeta *>(
        (buffer->data() +
         FileSystem::Instance()->super_meta()->file_block_meta_size_ * i));
    meta->used_ = false;
    meta->fh_ = -1;
    meta->index_ = i;
    meta->crc_ =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              FileSystem::Instance()->super_meta()->file_block_meta_size_ -
                  sizeof(meta->crc_));
  }
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      buffer->data(),
      FileSystem::Instance()->super_meta()->file_block_meta_total_size_,
      FileSystem::Instance()->super_meta()->file_block_meta_offset_);
  if (ret !=
      static_cast<int64_t>(
          FileSystem::Instance()->super_meta()->file_block_meta_total_size_)) {
    LOG(ERROR) << "write file block meta error size:" << ret;
    return false;
  }
  SPDLOG_INFO("write all file block meta success");
  return true;
}

FileBlockPtr FileBlockHandle::GetFileBlockLock() {
  META_HANDLE_LOCK();
  if (free_fbhs_.empty()) [[unlikely]] {
    LOG(ERROR) << "file block is exhausted";
    return nullptr;
  }

  uint32_t index = free_fbhs_.front();
  free_fbhs_.pop_front();
  FileBlockMeta *meta = reinterpret_cast<FileBlockMeta *>(
      base_addr() +
      FileSystem::Instance()->super_meta()->file_block_meta_size_ * index);
  assert(meta->used_ == false);
  assert(meta->used_block_num_ == 0);
  return std::make_shared<FileBlock>(index, meta);
}

}