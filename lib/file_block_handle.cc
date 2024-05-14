#include <assert.h>

#include "crc.h"
#include "file_store_udisk.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

FileBlockHandle::FileBlockHandle() {}
FileBlockHandle::~FileBlockHandle() {}

bool FileBlockHandle::InitializeMeta() {
  LOG(DEBUG)
      << "total file block num: "
      << FileStore::Instance()->super_meta()->max_support_file_block_num_;
  FileBlockMeta *meta;
  for (uint32_t index = 0;
       index < FileStore::Instance()->super_meta()->max_support_file_block_num_;
       ++index) {
    meta = reinterpret_cast<FileBlockMeta *>(base_addr() +
        FileStore::Instance()->super_meta()->file_block_meta_size_ * index);
    uint32_t crc =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              FileStore::Instance()->super_meta()->file_block_meta_size_ -
                  sizeof(meta->crc_));
    if (unlikely(meta->crc_ != crc)) {
      LOG(ERROR) << "debug file block meta: \n"
                 << " file block index: " << index << "\n"
                 << " crc error\n"
                 << " read crc: " << meta->crc_ << "\n"
                 << " cal crc: " << crc << "\n"
                 << " used: " << meta->used_ << "\n"
                 << " seq_no: " << meta->seq_no_ << "\n"
                 << " fh: " << meta->fh_ << "\n"
                 << " file_cut: " << meta->file_cut_ << "\n"
                 << " padding: " << meta->padding_ << "\n"
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
          FileStore::Instance()->file_handle()->GetCreatedFileNoLock(meta->fh_);
      if (unlikely(!file)) {
        LOG(ERROR) << "file block meta: " << index
                   << " invalid for fh: " << meta->fh_
                   << " file_cut: " << meta->file_cut_
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
        if (!FileStore::Instance()->block_handle()->GetSpecificBlockId(
                fb->get_block_id(i))) {
          return false;
        }
      }
      file->AddFileBlockNoLock(fb);
    } else {
      PutFileBlockNoLock(index);
    }
  }
  LOG(DEBUG) << "read file block meta success, free num:" << GetFreeMetaNum();
  return true;
}

bool FileBlockHandle::FormatAllMeta() {
  uint64_t file_block_meta_total_size =
      FileStore::Instance()->super_meta()->file_block_meta_total_size_;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      file_block_meta_total_size, FileStore::Instance()->dev()->block_size());

  FileBlockMeta *meta;
  for (uint32_t i = 0;
       i < FileStore::Instance()->super_meta()->max_support_file_block_num_;
       ++i) {
    meta = reinterpret_cast<FileBlockMeta *>(
        (buffer->data() +
         FileStore::Instance()->super_meta()->file_block_meta_size_ * i));
    meta->used_ = false;
    meta->fh_ = -1;
    meta->index_ = i;
    meta->crc_ =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              FileStore::Instance()->super_meta()->file_block_meta_size_ -
                  sizeof(meta->crc_));
  }
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      buffer->data(),
      FileStore::Instance()->super_meta()->file_block_meta_total_size_,
      FileStore::Instance()->super_meta()->file_block_meta_offset_);
  if (ret !=
      static_cast<int64_t>(
          FileStore::Instance()->super_meta()->file_block_meta_total_size_)) {
    LOG(ERROR) << "write file block meta error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all file block meta success";
  return true;
}

bool FileBlockHandle::GetFileBlockLock(uint32_t file_block_num,
                                       std::vector<FileBlockPtr> *file_blocks) {
  META_HANDLE_LOCK();
  if (unlikely(free_metas_.empty() || free_metas_.size() < file_block_num)) {
    LOG(ERROR) << "file block meta not enough";
    return false;
  }
  file_blocks->clear();

  FileBlockMeta *meta = nullptr;
  for (uint32_t i = 0; i < file_block_num; ++i) {
    uint32_t index = free_metas_.front();
    free_metas_.pop_front();
    meta = reinterpret_cast<FileBlockMeta *>(
        base_addr() +
        FileStore::Instance()->super_meta()->file_block_meta_size_ * index);
    assert(meta->used_ == false);
    assert(meta->used_block_num_ == 0);
    file_blocks->push_back(std::make_shared<FileBlock>(index, meta));
  }
  return true;
}

void FileBlockHandle::PutFileBlockLock(uint32_t index) {
  META_HANDLE_LOCK();
  PutFileBlockNoLock(index);
}

void FileBlockHandle::PutFileBlockNoLock(uint32_t index) {
  free_metas_.push_back(index);
}

bool FileBlockHandle::PutFileBlockLock(
    const std::vector<FileBlockPtr> &file_blocks) {
  META_HANDLE_LOCK();
  return PutFileBlockNoLock(file_blocks);
}

bool FileBlockHandle::PutFileBlockNoLock(
    const std::vector<FileBlockPtr> &file_blocks) {
  if (unlikely(file_blocks.size() == 0)) {
    LOG(ERROR) << "file block list empty";
    return false;
  }
  LOG(INFO) << "current file block size: " << free_metas_.size();
  for (uint32_t i = 0; i < file_blocks.size(); ++i) {
    free_metas_.push_back(file_blocks[i]->index());
    LOG(INFO) << "put file block index: " << file_blocks[i]->index()
              << " to free meta done";
  }
  LOG(INFO) << "current file block size: " << free_metas_.size();
  return true;
}

void FileBlockHandle::Dump() noexcept {}

void FileBlockHandle::Dump(const std::string &file_name) noexcept {}
}  // namespace blockfs
}  // namespace udisk