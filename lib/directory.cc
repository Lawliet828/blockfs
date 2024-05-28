#include "directory.h"

#include "file_store_udisk.h"
#include "logging.h"

// common dir/file functions
void Directory::stat(struct stat *buf) {
  FileSystem::Instance()->dir_handle()->RunInMetaGuard([this, buf] {
    /* initialize all the values */
    buf->st_dev = 0;                     /* ID of device containing file */
    buf->st_ino = dh();                  /* inode number */
    buf->st_mode = 0755;                 /* protection */
    buf->st_nlink = 0;                   /* number of hard links */
    buf->st_uid = ::getuid();            /* user ID of owner */
    buf->st_gid = ::getgid();            /* group ID of owner */
    buf->st_rdev = 0;                    /* device ID (if special file) */
    buf->st_blksize = kBlockFsBlockSize; /* blocksize for file system I/O */
    buf->st_blocks = 0;                  /* number of 512B blocks allocated */
    buf->st_atime = atime();             /* time of last access */
    buf->st_mtime = mtime();             /* time of last modification */
    buf->st_ctime = ctime();             /* time of last status change */
    buf->st_size = meta_->size_;         /* total size, in bytes */
    buf->st_mode |= S_IFDIR; /* specify whether item is a file or directory */
    return true;
  });
  return;
}

void Directory::UpdateTimeStamp(bool a, bool m, bool c) {
  TimeStamp ts = TimeStamp::now();
  int64_t microSecondsSinceEpoch = ts.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch /
                                       TimeStamp::kMicroSecondsPerSecond);
  if (a) set_atime(seconds);
  if (m) set_mtime(seconds);
  if (c) set_ctime(seconds);

  // 更新内存元数据的CRC
  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta_->crc_));
}

void Directory::IncLinkCount() noexcept {
  META_HANDLE_LOCK();
  ++nlink_;
}
void Directory::DecLinkCount() noexcept {
  META_HANDLE_LOCK();
  --nlink_;
}

const uint32_t Directory::LinkCount() noexcept {
  META_HANDLE_LOCK();
  return nlink_;
}

/**
 * 清理文件夹元数据字段
 *
 * \param none
 *
 * \return void
 */
void Directory::ClearMeta() const noexcept {
  meta_->used_ = false;
  // 后面从内存删除还需要用文件夹名,这个地方只置位回收状态
  // meta_->seq_no_ = kReservedUnusedSeq;
  // ::memset(meta_->dir_name_, 0, sizeof(meta_->dir_name_));
  meta_->size_ = 0;
  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta_->crc_));
}

void Directory::ClearMeta(dh_t dh) noexcept {
  DirMeta *meta = reinterpret_cast<DirMeta *>(
      FileSystem::Instance()->dir_handle()->base_addr() +
      FileSystem::Instance()->super_meta()->dir_meta_size_ * dh);
  meta->used_ = false;
  meta->size_ = 0;
  meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta->crc_));
}

/**
 * 删除文件夹, 回收文件夹元数据
 * 需要在文件元数据修改的保护锁内
 *
 * \param none
 *
 * \return success or failed
 */
bool Directory::Suicide() {
  return FileSystem::Instance()->dir_handle()->RunInMetaGuard([this] {
    return SuicideNolock();
  });
}
bool Directory::SuicideNolock() {
  ClearMeta();
  return WriteMeta();
}

/**
 * 返回文件夹目录下的文件和文件夹的个数总和
 *
 * \param none
 *
 * \return strip success or failed
 */
const uint32_t Directory::ChildCount() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return (item_maps_.size() + child_dir_maps_.size());
}

void Directory::ScanDir(std::vector<blockfs_dirent *> &dir_infos) {
  LOG(INFO) << "init scan directory: " << dir_name();
  for (const auto &d : child_dir_maps_) {
    LOG(INFO) << "scan directory: " << d.second->dir_name();
    blockfs_dirent *info = new blockfs_dirent();
    info->d_ino = d.second->dh();
    info->d_type = DT_DIR;
    // 全路径: 去掉'/', 返回整齐的名字
    std::string dir_name = d.second->dir_name();
    if (dir_name[dir_name.size() - 1] == '/') {
      dir_name[dir_name.size() - 1] = '\0';
    }
    dir_name = GetFileName(dir_name);
    ::memset(info->d_name, 0, sizeof(info->d_name));
    ::memcpy(info->d_name, dir_name.c_str(), dir_name.size());
    info->d_reclen = 0;
    info->d_time_ = d.second->atime();
    dir_infos.push_back(info);
  }
  for (const auto &item : item_maps_) {
    const FilePtr &file = item.second;
    LOG(INFO) << "scan file: " << file->file_name();
    blockfs_dirent *info = new blockfs_dirent();
    info->d_ino = file->fh();
    info->d_type = DT_REG;
    ::memcpy(info->d_name, file->file_name().c_str(), file->file_name().size());
    info->d_reclen = file->file_size();
    info->d_time_ = file->atime();
    dir_infos.push_back(info);
  }
}

void Directory::DumpMeta() {
  LOG(INFO) << "directory info: "
            << " dh: " << dh() << " dir_name: " << dir_name();
}

bool Directory::WriteMeta() {
  uint32_t align_index =
      FileSystem::Instance()->dir_handle()->PageAlignIndex(dh());
  LOG(INFO) << "directory handle: " << dh() << " align_index: " << align_index;
  uint64_t offset =
      FileSystem::Instance()->super_meta()->dir_meta_size_ * align_index;
  void *align_meta = FileSystem::Instance()->dir_handle()->base_addr() + offset;
  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta_->crc_));
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      align_meta, kBlockFsPageSize,
      FileSystem::Instance()->super_meta()->dir_meta_offset_ + offset);
  if (ret != kBlockFsPageSize) [[unlikely]] {
    LOG(ERROR) << "write directory meta " << dh() << "error size:" << ret
               << " need:" << kBlockFsPageSize;
    return false;
  }
  return true;
}

bool Directory::WriteMeta(dh_t dh) {
  DirMeta *meta = reinterpret_cast<DirMeta *>(
      FileSystem::Instance()->dir_handle()->base_addr() +
      FileSystem::Instance()->super_meta()->dir_meta_size_ * dh);
  uint32_t align_index =
      FileSystem::Instance()->dir_handle()->PageAlignIndex(dh);
  uint64_t offset =
      FileSystem::Instance()->super_meta()->dir_meta_size_ * align_index;
  void *align_meta = FileSystem::Instance()->dir_handle()->base_addr() + offset;
  meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta->crc_));
  LOG(INFO) << "write dir meta, name: " << meta->dir_name_ << " dh: " << dh
            << " align_index: " << align_index << " crc: " << meta->crc_;
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      align_meta, kBlockFsPageSize,
      FileSystem::Instance()->super_meta()->dir_meta_offset_ + offset);
  if (unlikely(ret != kBlockFsPageSize)) {
    LOG(ERROR) << "write directory meta " << dh << "error size:" << ret
               << " need:" << kBlockFsPageSize;
    return false;
  }
  return true;
}

bool Directory::AddChildFile(const FilePtr &file) {
  std::lock_guard<std::mutex> lock(mutex_);
  return AddChildFileNoLock(file);
}

bool Directory::AddChildFileNoLock(const FilePtr &file) {
  if (item_maps_.find(file->fh()) != item_maps_.end()) {
    LOG(ERROR) << "file fh has exist, name: " << file->file_name();
    return false;
  }
  LOG(DEBUG) << "add fh: " << file->fh() << " name: " << file->file_name()
             << " to " << dir_name();
  item_maps_[file->fh()] = file;
  return true;
}

bool Directory::RemoveChildFile(const FilePtr &file) {
  std::lock_guard<std::mutex> lock(mutex_);
  return RemoveChildFileNolock(file);
}

bool Directory::RemoveChildFileNolock(const FilePtr &file) {
  if (item_maps_.erase(file->fh()) != 1) {
    LOG(ERROR) << "failed to remove child file: " << file->file_name();
    return false;
  }
  return true;
}

bool Directory::AddChildDirectoryNolock(const DirectoryPtr &child) {
  if (child_dir_maps_.find(child->dh()) != child_dir_maps_.end()) {
    LOG(ERROR) << "directory dh has exist, name: " << child->dir_name();
    return false;
  }
  child_dir_maps_[child->dh()] = child;
  return true;
}

bool Directory::AddChildDirectory(const DirectoryPtr &child) {
  std::lock_guard<std::mutex> lock(mutex_);
  return AddChildDirectoryNolock(child);
}

bool Directory::RemovChildDirectory(const DirectoryPtr &child) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (child_dir_maps_.erase(child->dh()) != 1) {
    LOG(ERROR) << "failed to remove child directory: " << child->dir_name();
    return false;
  }
  return true;
}

bool Directory::ForceRemoveAllFiles() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unordered_map<int32_t, FilePtr>::iterator it;
  LOG(INFO) << "file num: " << item_maps_.size();
  for (it = item_maps_.begin(); it != item_maps_.end();) {
    LOG(INFO) << "file index: " << it->first
              << " name: " << it->second->file_name();
    FileSystem::Instance()->file_handle()->unlink(dir_name() +
                                                 it->second->file_name());
    item_maps_.erase(it++);
  }
  // TOFO: remote  all dirs
  // child_dir_maps_.clear();
  return true;
}

int Directory::rename(const std::string &to) {
  bool success =
      FileSystem::Instance()->dir_handle()->RunInMetaGuard([this, to] {
        LOG(INFO) << "rename old dir: " << dir_name() << " to: " << to;
        set_dir_name(to);
        if (!WriteMeta()) {
          return false;
        }
        return true;
      });
  // TOOD: 文件夹的dh没有改变
  // TODO: 修改打开文件name的map关系
  return success ? 0 : -1;
}
