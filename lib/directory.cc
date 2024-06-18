#include "directory.h"

#include <chrono>

#include "file_system.h"
#include "spdlog/spdlog.h"

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
    buf->st_blksize = kBlockSize; /* blocksize for file system I/O */
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
  using namespace std::chrono;
  system_clock::time_point now = system_clock::now();
  time_t seconds = system_clock::to_time_t(now);
  if (a) set_atime(seconds);
  if (m) set_mtime(seconds);
  if (c) set_ctime(seconds);

  // 更新内存元数据的CRC
  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta_->crc_));
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
uint32_t Directory::ChildCount() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return (item_maps_.size() + child_dir_maps_.size());
}

void Directory::ScanDir(std::vector<block_fs_dirent *> &dir_infos) {
  SPDLOG_INFO("init scan directory: {}", dir_name());
  for (const auto &d : child_dir_maps_) {
    SPDLOG_INFO("scan directory: {}", d.second->dir_name());
    block_fs_dirent *info = new block_fs_dirent();
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
    SPDLOG_INFO("scan file: {}", file->file_name());
    block_fs_dirent *info = new block_fs_dirent();
    info->d_ino = file->fh();
    info->d_type = DT_REG;
    ::memcpy(info->d_name, file->file_name().c_str(), file->file_name().size());
    info->d_reclen = file->file_size();
    info->d_time_ = file->atime();
    dir_infos.push_back(info);
  }
}

void Directory::DumpMeta() {
  SPDLOG_INFO("directory info: dh: {} dir_name: {}", dh(), dir_name());
}

bool Directory::WriteMeta() {
  uint32_t align_index =
      FileSystem::Instance()->dir_handle()->PageAlignIndex(dh());
  SPDLOG_INFO("directory handle: {} align_index: {}", dh(), align_index);
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
    SPDLOG_ERROR("write directory meta {} error size: {} need: {}", dh(), ret,
                 kBlockFsPageSize);
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
  uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                      FileSystem::Instance()->super_meta()->dir_meta_size_ -
                          sizeof(meta->crc_));
  meta->crc_ = crc;
  SPDLOG_INFO("write dir meta, name: {} dh: {} align_index: {} crc: {}",
              meta->dir_name_, dh, align_index, crc);
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      align_meta, kBlockFsPageSize,
      FileSystem::Instance()->super_meta()->dir_meta_offset_ + offset);
  if (ret != kBlockFsPageSize) [[unlikely]] {
    SPDLOG_ERROR("write directory meta {} error size: {} need: {}", dh, ret,
                 kBlockFsPageSize);
    return false;
  }
  return true;
}

bool Directory::AddChildFile(const FilePtr &file) {
  std::lock_guard<std::mutex> lock(mutex_);
  return AddChildFileNoLock(file);
}

bool Directory::AddChildFileNoLock(const FilePtr &file) {
  if (item_maps_.contains(file->fh())) {
    SPDLOG_ERROR("file fh has exist, name: {}", file->file_name());
    return false;
  }
  SPDLOG_DEBUG("add fh: {} name: {} to {}", file->fh(), file->file_name(),
               dir_name());
  item_maps_[file->fh()] = file;
  return true;
}

bool Directory::RemoveChildFile(const FilePtr &file) {
  std::lock_guard<std::mutex> lock(mutex_);
  return RemoveChildFileNolock(file);
}

bool Directory::RemoveChildFileNolock(const FilePtr &file) {
  if (item_maps_.erase(file->fh()) != 1) {
    SPDLOG_ERROR("failed to remove child file: {}", file->file_name());
    return false;
  }
  return true;
}

bool Directory::AddChildDirectoryNolock(const DirectoryPtr &child) {
  if (child_dir_maps_.find(child->dh()) != child_dir_maps_.end()) {
    SPDLOG_ERROR("directory dh has exist, name: {}", child->dir_name());
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
    SPDLOG_ERROR("failed to remove child directory: {}", child->dir_name());
    return false;
  }
  return true;
}

bool Directory::ForceRemoveAllFiles() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unordered_map<int32_t, FilePtr>::iterator it;
  SPDLOG_INFO("file num: {}", item_maps_.size());
  for (it = item_maps_.begin(); it != item_maps_.end();) {
    SPDLOG_INFO("file index: {} name: {}", it->first,
                it->second->file_name());
    FileSystem::Instance()->file_handle()->unlink(dir_name() +
                                                 it->second->file_name());
    item_maps_.erase(it++);
  }
  // TOFO: remote  all dirs
  // child_dir_maps_.clear();
  return true;
}

/*
实际测试mv命令会报错mv: 无法获取'dir11' 的文件状态(stat): No such file or directory
看日志是在rename成功后, 又getattr原路径
但是我在upfs中操作, 没有这个问题
*/
int Directory::rename(const std::string &to) {
  bool success =
      FileSystem::Instance()->dir_handle()->RunInMetaGuard([this, to] {
        SPDLOG_INFO("rename dir {} -> {}", dir_name(), to);

        std::string new_parent_dir_name = GetParentDirName(to);
        const DirectoryPtr &new_parent_dir =
            FileSystem::Instance()->dir_handle()->GetCreatedDirectoryNolock(new_parent_dir_name);
        if (!new_parent_dir) {
          SPDLOG_ERROR("new parent dir {} not exist", new_parent_dir_name);
          return false;
        }

        std::string old_parent_dir_name = GetParentDirName(dir_name());
        const DirectoryPtr &old_parent_dir =
            FileSystem::Instance()->dir_handle()->GetCreatedDirectoryNolock(old_parent_dir_name);
        if (!old_parent_dir) {
          SPDLOG_ERROR("old parent dir {} not exist", old_parent_dir_name);
          return false;
        }
        old_parent_dir->RemovChildDirectory(shared_from_this());
        if (!FileSystem::Instance()->dir_handle()->RemoveDirectoryFromCreateNolock(shared_from_this())) {
          return false;
        }

        set_dir_name(to);

        FileSystem::Instance()->dir_handle()->AddDirectory2CreateNolock(shared_from_this());
        new_parent_dir->AddChildDirectory(shared_from_this());

        if (!WriteMeta()) {
          return false;
        }
        return true;
      });
  return success ? 0 : -1;
}
