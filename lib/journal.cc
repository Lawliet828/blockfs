#include "journal.h"

#include "file_store_udisk.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

void Journal::set_create_file(const FilePtr &file) const noexcept {
  ::memcpy(&meta_->create_file_.file_meta_, file->meta(),
           FileStore::Instance()->super_meta()->file_meta_size_);
}
void Journal::set_delete_file(const FilePtr &file) const noexcept {
  meta_->delete_file_.fh_ = file->fh();
}
void Journal::set_expand_file(const FilePtr &file) const noexcept {
  meta_->expand_file_.fh_ = file->fh();
  meta_->expand_file_.size_ = file->file_size();
}

void Journal::set_expand_file_ex(FileBlockPtr last_file_block,
                      const std::vector<FileBlockPtr> &file_blocks) const {
  if (nullptr != last_file_block) {
    meta_->expand_file_.last_file_block_index_ = last_file_block->index();
    meta_->expand_file_.used_block_num_ = last_file_block->used_block_num();
  }
  meta_->expand_file_.file_block_num_ = file_blocks.size();
  ::memset(meta_->expand_file_.file_block_ids_, 0, sizeof(meta_->expand_file_.file_block_ids_));
  int i = 0;
  for (auto file_block : file_blocks) {
    meta_->expand_file_.file_block_ids_[i++] = file_block->index();
  }
}

void Journal::set_truncate_file(const FilePtr &file) const noexcept {
  ::memcpy(&meta_->truncate_file_.parent_file_meta_, file->meta(),
           FileStore::Instance()->super_meta()->file_meta_size_);
  meta_->truncate_file_.new_fh_ = file->child_fh();
}

void Journal::set_rename_file(const FilePtr &file) const noexcept {
  meta_->rename_file_.fh_ = file->fh();
  ::memset(meta_->rename_file_.file_name_, 0, sizeof(meta_->rename_file_.file_name_));
  ::memcpy(meta_->rename_file_.file_name_, file->file_name().c_str(),
           sizeof(meta_->rename_file_.file_name_));
  meta_->rename_file_.dh_ = file->dh();
  meta_->rename_file_.atime_ = file->atime();
  meta_->rename_file_.mtime_ = file->mtime();
  meta_->rename_file_.ctime_ = file->ctime();
}

void Journal::set_create_directory(const DirectoryPtr &dir) const noexcept {
  ::memcpy(&meta_->create_dir_.dir_meta_, dir->meta(),
           FileStore::Instance()->super_meta()->dir_meta_size_);
}
void Journal::set_delete_directory(const DirectoryPtr &dir) const noexcept {
  meta_->delete_dir_.dh_ = dir->dh();
}

bool Journal::WriteMeta() {
  meta_->crc_ =
      Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
            FileStore::Instance()->super_meta()->block_fs_journal_size_ -
                sizeof(meta_->crc_));
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      meta_, FileStore::Instance()->super_meta()->block_fs_journal_size_,
      FileStore::Instance()->super_meta()->block_fs_journal_offset_ +
          FileStore::Instance()->super_meta()->block_fs_journal_size_ * index_);
  if (unlikely(ret != static_cast<int64_t>(
              FileStore::Instance()->super_meta()->block_fs_journal_size_))) {
    LOG(ERROR) << "Write journal: " << index_ << " failed, ret: " << ret;
    return false;
  }
  return true;
}

bool Journal::ReplayCreateFile() {
  FileMeta *file_meta = &meta_->create_file_.file_meta_;
  int32_t dh = file_meta->dh_;
  FileNameKey key = std::make_pair(dh, file_meta->file_name_);
  const FilePtr &file =
      FileStore::Instance()->file_handle()->GetCreatedFileNoLock(key);
  if (!file) {
    const DirectoryPtr &dir =
        FileStore::Instance()->dir_handle()->GetCreatedDirectoryNolock(
            file_meta->dh_);
    if (!dir) {
      return false;
    }
    if (!FileStore::Instance()->file_handle()->CreateFile(
            dir->dir_name() + file_meta->file_name_, 0)) {
      return false;
    }
  }
  return true;
}

bool Journal::ReplayDeleteFile() {
  int32_t fh = meta_->delete_file_.fh_;
  int ret = FileStore::Instance()->file_handle()->UnlinkFile(fh);
  return 0 == ret;
}

bool Journal::ReplayTruncateFile() {
  // 删除child file
  FileStore::Instance()->file_handle()->UnlinkFile(meta_->truncate_file_.new_fh_);
  return true;
}

bool Journal::ReplayRenameFile() { return true; }

bool Journal::ReplayExpandFile() {
  return true;
}

bool Journal::ReplayDeleteDirectory() {
  int32_t dh = meta_->delete_dir_.dh_;
  int ret = FileStore::Instance()->dir_handle()->DeleteDirectory(dh);
  return 0 == ret;
}

bool Journal::Replay() {
  switch (meta_->type_) {
    case BLOCKFS_JOURNAL_CREATE_FILE: {
      return ReplayCreateFile();
    } break;
    case BLOCKFS_JOURNAL_DELETE_FILE: {
      return ReplayDeleteFile();
    } break;
    case BLOCKFS_JOURNAL_TRUNCATE_FILE: {
      return ReplayTruncateFile();
    } break;
    case BLOCKFS_JOURNAL_RENAME_FILE: {
      return ReplayRenameFile();
    } break;
    case BLOCKFS_JOURNAL_EXPAND_FILE: {
      return ReplayExpandFile();
    } break;
    case BLOCKFS_JOURNAL_CREATE_DIR: {
      return true;
    } break;
    case BLOCKFS_JOURNAL_DELETE_DIR: {
      return ReplayDeleteDirectory();
    } break;
    default: {
      return false;
    } break;
  }
  return true;
}

bool Journal::PreHandleTruncateFile() {
  LOG(DEBUG) << "enter PreHandleTruncateFile";
  // 将日志中的元数据拷贝进内存
  FileMeta *file_meta = &meta_->truncate_file_.parent_file_meta_;
  int32_t fh = file_meta->fh_;
  file_meta->child_fh_ = -1;
  FileMeta *dest_meta = reinterpret_cast<FileMeta *>(
                      FileStore::Instance()->file_handle()->base_addr() +
                      FileStore::Instance()->super_meta()->file_meta_size_ * fh);
  ::memcpy(dest_meta, file_meta,
           FileStore::Instance()->super_meta()->file_meta_size_);
  // TODO WriteMeta

  LOG(DEBUG) << "PreHandleTruncateFile success";
  return true;
}

bool Journal::PreHandleRenameFile() {
  LOG(DEBUG) << "enter PreHandleRenameFile";
  int32_t fh = meta_->rename_file_.fh_;
  FileMeta *meta = reinterpret_cast<FileMeta *>(
                  FileStore::Instance()->file_handle()->base_addr() +
                  FileStore::Instance()->super_meta()->file_meta_size_ * fh);
  // 将内存中的元数据更新为rename后的新数据
  ::memset(meta->file_name_, 0, sizeof(meta->file_name_));
  ::memcpy(meta->file_name_, meta_->rename_file_.file_name_,
           sizeof(meta->file_name_));
  meta->dh_ = meta_->rename_file_.dh_;
  meta->atime_ = meta_->rename_file_.atime_;
  meta->mtime_ = meta_->rename_file_.mtime_;
  meta->ctime_ = meta_->rename_file_.ctime_;
  // 写入磁盘
  FilePtr file = std::make_shared<File>();
  if (!file) {
    LOG(ERROR) << "failed to new file pointer";
    block_fs_set_errno(ENOMEM);
    return false;
  }
  file->set_meta(meta);
  file->set_used(true);
  file->set_fh(fh);
  file->set_dh(meta->dh_);
  file->set_file_name(meta->file_name_);
  file->set_is_temp(false);
  if (!file->WriteMeta()) {
    return false;
  }
  LOG(DEBUG) << "pre handle rename file success, fh: " << fh
             << " file name: " << meta->file_name_;
  return true;
}

bool Journal::PreHandleExpandFile() {
  LOG(DEBUG) << "enter PreHandleExpandFile";
  int32_t fh = meta_->expand_file_.fh_;
  FileMeta *meta = reinterpret_cast<FileMeta *>(
                  FileStore::Instance()->file_handle()->base_addr() +
                  FileStore::Instance()->super_meta()->file_meta_size_ * fh);
  // 文件大小还原为原大小
  meta->size_ = meta_->expand_file_.size_;
  // 写入磁盘
  File::WriteMeta(fh);
  // 修复last file block
  uint32_t index = meta_->expand_file_.last_file_block_index_;
  FileBlockMeta *last_file_block_meta = reinterpret_cast<FileBlockMeta *>(
        FileStore::Instance()->file_block_handle()->base_addr() +
        FileStore::Instance()->super_meta()->file_block_meta_size_ * index);
  assert(last_file_block_meta->used_);
  last_file_block_meta->used_block_num_ = meta_->expand_file_.used_block_num_;
  for (uint32_t i = last_file_block_meta->used_block_num_; i < kBlockFsFileBlockCapacity; ++i) {
    last_file_block_meta->block_id_[i] = 0;
  }
  FileBlock::WriteMeta(index);
  // 修复扩展的file block
  for (uint32_t i = 0; i < meta_->expand_file_.file_block_num_; ++i) {
    uint32_t idx = meta_->expand_file_.file_block_ids_[i];
    FileBlockMeta *file_block_meta = reinterpret_cast<FileBlockMeta *>(
          FileStore::Instance()->file_block_handle()->base_addr() +
          FileStore::Instance()->super_meta()->file_block_meta_size_ * idx);
    FileBlock::ClearMeta(file_block_meta);
    if (!FileBlock::WriteMeta(idx)) {
      return false;
    }
  }
  return true;
}

bool Journal::PreHandleCreateDir() {
  LOG(DEBUG) << "enter PreHandleCreateDir";
  DirMeta *dir_meta = &meta_->create_dir_.dir_meta_;
  int32_t dh = dir_meta->dh_;
  uint64_t dir_meta_size = FileStore::Instance()->super_meta()->dir_meta_size_;
  // 查询journal中是否有创建同名文件夹
  dh_t pre_dh = FileStore::Instance()->journal_handle()->GetReplayCreateDir(dir_meta->dir_name_);
  if (unlikely(-1 != pre_dh)) {
    // journal中有创建同名文件夹
    DirMeta *pre_dir_meta = reinterpret_cast<DirMeta *>(
                FileStore::Instance()->dir_handle()->base_addr() +
                dir_meta_size * pre_dh);
    if (pre_dir_meta->seq_no_ < dir_meta->seq_no_) {
      // 当前dir meta舍去
      Directory::ClearMeta(dh);
      if (!Directory::WriteMeta(dh)) {
        return false;
      }
      return true;
    } else if (pre_dir_meta->seq_no_ == dir_meta->seq_no_) {
      LOG(ERROR) << "dir name: " << pre_dir_meta->dir_name_ << " dh: " << pre_dir_meta->dh_
                << " dir name: " << dir_meta->dir_name_ << " dh: " << dir_meta->dh_
                << " has same seq_no: " << dir_meta->seq_no_;
      return false;
    } else {
      // 前一个dir meta清除
      Directory::ClearMeta(pre_dh);
      if (!Directory::WriteMeta(pre_dh)) {
        return false;
      }
      // 持久化当前dir meta
      DirMeta *meta = reinterpret_cast<DirMeta *>(
                    FileStore::Instance()->dir_handle()->base_addr() +
                    dir_meta_size * dh);
      ::memcpy(meta, dir_meta, dir_meta_size);
      if (!Directory::WriteMeta(dh)) {
        return false;
      }
    }
    // 设置replay_create_dirs_
    FileStore::Instance()->journal_handle()->SetReplayCreateDir(dir_meta->dir_name_, dh);
  } else {
    // journal中没有创建同名文件夹
    // 持久化元数据
    DirMeta *meta = reinterpret_cast<DirMeta *>(
                  FileStore::Instance()->dir_handle()->base_addr() +
                  dir_meta_size * dh);
    ::memcpy(meta, dir_meta, dir_meta_size);
    if (!Directory::WriteMeta(dh)) {
      return false;
    }
    // 设置replay_create_dirs_
    FileStore::Instance()->journal_handle()->SetReplayCreateDir(dir_meta->dir_name_, dh);
  }

  return true;
}

bool Journal::PreHandle() {
  switch (meta_->type_) {
    case BLOCKFS_JOURNAL_CREATE_FILE: {
      return true;
    } break;
    case BLOCKFS_JOURNAL_DELETE_FILE: {
      return true;
    } break;
    case BLOCKFS_JOURNAL_EXPAND_FILE: {
      return PreHandleExpandFile();
    } break;
    case BLOCKFS_JOURNAL_TRUNCATE_FILE: {
      return PreHandleTruncateFile();
    } break;
    case BLOCKFS_JOURNAL_RENAME_FILE: {
      return PreHandleRenameFile();
    } break;
    case BLOCKFS_JOURNAL_CREATE_DIR: {
      return PreHandleCreateDir();
    } break;
    case BLOCKFS_JOURNAL_DELETE_DIR: {
      return true;
    } break;
    default: {
      return false;
    } break;
  }
  return true;
}

bool Journal::Recycle() {
  uint64_t jh = index();
  ::memset(meta_, 0,
           FileStore::Instance()->super_meta()->block_fs_journal_size_);
  meta_->jh_ = jh;
  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      sizeof(BlockFsJournal) - sizeof(meta_->crc_));
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      meta_, FileStore::Instance()->super_meta()->block_fs_journal_size_,
      FileStore::Instance()->super_meta()->block_fs_journal_offset_ +
          FileStore::Instance()->super_meta()->block_fs_journal_size_ * index_);
  if (unlikely(ret != static_cast<int64_t>(
              FileStore::Instance()->super_meta()->block_fs_journal_size_))) {
    LOG(ERROR) << "Recycle journal: " << index_ << " failed, ret: " << ret;
    return false;
  }
  return true;
}

}  // namespace blockfs
}  // namespace udisk