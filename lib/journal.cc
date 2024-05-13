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