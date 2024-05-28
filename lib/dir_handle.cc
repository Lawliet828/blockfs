#include <assert.h>
#include <libgen.h>
#include <stdio.h>

#include "crc.h"
#include "file_system.h"
#include "logging.h"

namespace udisk::blockfs {

const static DirectoryPtr kEmptyDirectoryPtr;

bool DirHandle::RunInMetaGuard(const DirectoryCallback &cb) {
  META_HANDLE_LOCK();
  return cb();
}

bool DirHandle::InitializeMeta() {
  DirMeta *meta;
  for (uint64_t dh = 0;
       dh < FileSystem::Instance()->super_meta()->max_file_num; ++dh) {
    meta = reinterpret_cast<DirMeta *>(
        base_addr() + FileSystem::Instance()->super_meta()->dir_meta_size_ * dh);
    uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                         FileSystem::Instance()->super_meta()->dir_meta_size_ -
                             sizeof(meta->crc_));
    if (meta->crc_ != crc) [[unlikely]] {
      LOG(ERROR) << "directory meta " << dh << " " << meta->dir_name_
                 << " crc error, read: " << meta->crc_ << " cal: " << crc;
      return false;
    }

    if (meta->used_) {
      LOG(DEBUG) << "directory handle: " << dh << " name: " << meta->dir_name_
                   << " seq_no: " << meta->seq_no_ << " crc: " << meta->crc_;
      if (::strnlen(meta->dir_name_, sizeof(meta->dir_name_)) == 0) [[unlikely]] {
        LOG(ERROR) << "directory meta " << dh << " used but name empty";
        return false;
      }
      if (meta->dh_ != static_cast<dh_t>(dh)) [[unlikely]] {
        LOG(ERROR) << "directory meta " << dh << " used but dh invalid";
        return false;
      }
      DirectoryPtr dir = std::make_shared<Directory>(meta);
      if (!dir || !AddDirectory2CreateNolock(dir)) {
        return false;
      }
    } else {
      AddDirectory2FreeNolock(dh);
    }
  }

  // 扫面所有文件夹,把子文件夹加入到父文件夹里面
  // 后面扫面文件的时候, 再把子文件加到父文件中.
  for (const auto &d : created_dirs_) {
    // 挂载点没有父目录
    if (unlikely(IsMountPoint(d.second->dir_name()))) {
      continue;
    }
    std::string dir_name = d.second->dir_name();
    std::string parent_dir_name = GetParentDirName(dir_name);
    LOG(DEBUG) << "add child directory: " << dir_name;
    const DirectoryPtr &parent_dir = GetCreatedDirectoryNolock(parent_dir_name);
    if (!parent_dir || !parent_dir->AddChildDirectoryNolock(d.second)) {
      return false;
    }
  }

  LOG(DEBUG) << "read directory meta success, free num:" << GetFreeMetaSize();
  return true;
}

/**
 * 通过索引找到对齐的4K页面的, 相同的偏移index只允许一个写元数据
 *
 * \param index 即将写的文件夹的元数据索引
 *
 * \return strip success or failed
 */
uint32_t DirHandle::PageAlignIndex(uint32_t index) const {
  static const uint32_t DIR_ALIGN =
      kBlockFsPageSize / FileSystem::Instance()->super_meta()->dir_meta_size_;
  return index - (index % DIR_ALIGN);
}

bool DirHandle::FormatAllMeta() {
  uint64_t dir_meta_total_size =
      FileSystem::Instance()->super_meta()->dir_meta_total_size_;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      dir_meta_total_size, FileSystem::Instance()->dev()->block_size());

  DirMeta *meta;
  for (uint64_t dh = 0;
       dh < FileSystem::Instance()->super_meta()->max_file_num; ++dh) {
    meta = reinterpret_cast<DirMeta *>(
        buffer->data() +
        FileSystem::Instance()->super_meta()->dir_meta_size_ * dh);
    meta->used_ = false;
    meta->dh_ = dh;
    meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                       FileSystem::Instance()->super_meta()->dir_meta_size_ -
                           sizeof(meta->crc_));
  }
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      buffer->data(), FileSystem::Instance()->super_meta()->dir_meta_total_size_,
      FileSystem::Instance()->super_meta()->dir_meta_offset_);
  if (ret != static_cast<int64_t>(
                 FileSystem::Instance()->super_meta()->dir_meta_total_size_)) {
    LOG(ERROR) << "write directory meta error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all directory meta success";
  return true;
}

bool DirHandle::TransformPath(const std::string &path, std::string &dir_name) {
  dir_name = path;
  /* 存储元数据的时候增加文件夹分隔符 */
  if (dir_name[dir_name.size() - 1] != '/') {
    dir_name += '/';
  }
  if (dir_name.size() >= kBlockFsMaxDirNameLen) {
    LOG(ERROR) << "directory path: " << dir_name
               << " too long, size: " << dir_name.size()
               << " max limit: " << kBlockFsMaxDirNameLen;
    errno = ENAMETOOLONG;
    return false;
  }
  // 创建挂载目录不需要检查
  if (!IsMountPoint(dir_name)) {
    return FileSystem::Instance()->super()->CheckMountPoint(dir_name);
  }
  return true;
}

/**
 * 判断需要操作的目录(带/分隔符)是否是根目录(挂载点)
 *
 * \param dirname UXDB-UDISK约定: DB创建的绝对目录
 *
 * \return strip success or failed
 */
bool DirHandle::IsMountPoint(const std::string &dirname) const noexcept {
  return FileSystem::Instance()->super()->is_mount_point(dirname);
}

/**
 * 检查是否有同名的文件名存在
 *
 * \param dirname UXDB-UDISK约定: DB创建的绝对目录
 *
 * \return strip success or failed
 */
bool DirHandle::CheckFileExist(const std::string &dirname,
                               bool check_parent) const noexcept {
  if (!IsMountPoint(dirname)) {
    std::string path = dirname;
    if (path[path.size() - 1] == '/') {
      path.erase(path.size() - 1);
    }
    // 检查同名的文件是否存在
    if (FileSystem::Instance()->file_handle()->GetCreatedFile(path)) {
      LOG(ERROR) << "the same file path exist: " << dirname;
      if (check_parent) {
        errno = EEXIST;
      } else {
        errno = ENOTDIR;
      }
      return false;
    }
    std::string parent_path = GetParentDirName(dirname);
    if (!IsMountPoint(parent_path) && check_parent) {
      if (parent_path[parent_path.size() - 1] == '/') {
        parent_path.erase(parent_path.size() - 1);
      }
      // 检查上一层是否目录是否存在并且为文件
      if (FileSystem::Instance()->file_handle()->GetCreatedFile(parent_path)) {
        LOG(ERROR) << "the same parent path file exist: " << dirname;
        block_fs_set_errno(ENOTDIR);
        return false;
      }
    }
  }
  return true;
}

/**
 * 分配新的文件夹的元数据
 *
 * \param dirname UXDB-UDISK约定: DB创建的绝对目录
 * \param dirs 返回父文件夹和新申请文件夹
 *
 * \return strip success or failed
 */
DirectoryPtr DirHandle::NewFreeDirectoryNolock(const std::string &dirname) {
  if (free_dhs_.empty()) [[unlikely]] {
    LOG(ERROR) << "directory meta not enough";
    errno = ENFILE;
    return nullptr;
  }
  dh_t dh = free_dhs_.front();
  LOG(INFO) << "create new directory handle: " << dh;

  DirMeta *meta =
      reinterpret_cast<DirMeta *>(base_addr() + sizeof(DirMeta) * dh);
  if (meta->used_ || meta->dh_ != dh) [[unlikely]] {
    LOG(FATAL) << "new directory meta invalid, used: " << meta->used_
               << " dh: " << meta->dh_ << " wanted dh: " << dh;
    errno = EINVAL;
    return nullptr;
  }

  DirectoryPtr dir = std::make_shared<Directory>();
  if (!dir) {
    LOG(ERROR) << "failed to new directory pointer";
    block_fs_set_errno(ENOMEM);
    return nullptr;
  }
  dir->set_meta(meta);
  dir->set_used(true);
  dir->set_dh(dh);
  dir->set_dir_name(dirname);
  time_t time = ::time(nullptr);
  dir->set_atime(time);
  dir->set_mtime(time);
  dir->set_ctime(time);

  // 确保最后完成才摘链
  free_dhs_.pop_front();
  return dir;
}

/**
 * 分配新的文件夹
 *
 * \param dirname UXDB-UDISK约定: DB创建的绝对目录
 * \param dirs 返回父文件夹和新申请文件夹
 *
 * \return strip success or failed
 */
bool DirHandle::NewDirectory(const std::string &dirname,
                             std::pair<DirectoryPtr, DirectoryPtr> *dirs) {
  META_HANDLE_LOCK();
  auto itor = created_dirs_.find(dirname);
  if (itor != created_dirs_.end()) [[unlikely]] {
    LOG(ERROR) << "directory has already exist: " << dirname;
    errno = EEXIST;
    return false;
  }
  if (!IsMountPoint(dirname)) {
    std::string parent_dir_name = GetParentDirName(dirname);
    itor = created_dirs_.find(parent_dir_name);
    if (itor == created_dirs_.end()) [[unlikely]] {
      LOG(ERROR) << "parent directory not exist: " << parent_dir_name;
      errno = ENOENT;
      return false;
    }
    // 找到父文件夹
    dirs->first = itor->second;
  }

  // 父目录是文件,不能在上面创建文件夹
  DirectoryPtr dir = NewFreeDirectoryNolock(dirname);
  if (!dir) {
    return false;
  }
  dirs->second = dir;
  return true;
}

void DirHandle::AddDirectory2FreeNolock(dh_t index) {
  free_dhs_.push_back(index);
}

void DirHandle::AddDirectory2Free(dh_t index) {
  META_HANDLE_LOCK();
  AddDirectory2FreeNolock(index);
}

bool DirHandle::AddDirectory2CreateNolock(const DirectoryPtr &child) {
  // 当前文件夹加入内存映射表
  if (created_dirs_.find(child->dir_name()) != created_dirs_.end()) {
    LOG(FATAL) << "directory name has exist, name: " << child->dir_name();
    return false;
  }
  if (created_dhs_.find(child->dh()) != created_dhs_.end()) {
    LOG(FATAL) << "directory dh has exist, name: " << child->dir_name();
    return false;
  }
  created_dirs_[child->dir_name()] = child;
  created_dhs_[child->dh()] = child;
  return true;
}

bool DirHandle::AddDirectory(const DirectoryPtr &parent,
                             const DirectoryPtr &child) {
  // 创建的目录不是挂载点的根目录才更新内存文件映射
  if (!IsMountPoint(child->dir_name())) {
    if (!parent->AddChildDirectory(child)) {
      return false;
    }
  }
  META_HANDLE_LOCK();
  if (!AddDirectory2CreateNolock(child)) {
    assert(!parent->RemovChildDirectory(child));
    return false;
  }
  return true;
}

bool DirHandle::FindDirectory(const std::string &dirname,
                              std::pair<DirectoryPtr, DirectoryPtr> *dirs) {
  META_HANDLE_LOCK();
  std::string parent_dir_name = GetParentDirName(dirname);
  auto itor = created_dirs_.find(parent_dir_name);
  if (unlikely(itor == created_dirs_.end())) {
    LOG(ERROR) << "parent directory not exist: " << parent_dir_name;
    block_fs_set_errno(ENOENT);
    return false;
  }
  dirs->first = itor->second;

  itor = created_dirs_.find(dirname);
  if (unlikely(itor == created_dirs_.end())) {
    LOG(ERROR) << "directory not exist: " << dirname;
    block_fs_set_errno(ENOENT);
    return false;
  }
  dirs->second = itor->second;
  return true;
}

bool DirHandle::RemoveDirectoryFromCreateNolock(const DirectoryPtr &child) {
  // 从创建文件的映射中删除
  if (created_dirs_.erase(child->dir_name()) != 1) {
    LOG(ERROR) << "failed to remove directory name map: " << child->dir_name();
    return false;
  }
  if (created_dhs_.erase(child->dh()) != 1) {
    LOG(ERROR) << "failed to remove directory dh map: " << child->dir_name();
    return false;
  }
  // deleted_dirs_[child->dir_name()] = child;
  return true;
}

bool DirHandle::RemoveDirectory(const DirectoryPtr &parent,
                                const DirectoryPtr &child) {
  if (!parent->RemovChildDirectory(child)) {
    return false;
  }
  META_HANDLE_LOCK();
  return RemoveDirectoryFromCreateNolock(child);
}

const DirectoryPtr &DirHandle::GetOpenDirectory(ino_t fd) {
  META_HANDLE_LOCK();
  auto itor = open_dirs_.find(fd);
  if (itor == open_dirs_.end()) [[unlikely]] {
    LOG(ERROR) << "directory not been opened: " << fd;
    return kEmptyDirectoryPtr;
  }
  return itor->second;
}

const DirectoryPtr &DirHandle::GetCreatedDirectory(dh_t dh) {
  META_HANDLE_LOCK();
  return GetCreatedDirectoryNolock(dh);
}

const DirectoryPtr &DirHandle::GetCreatedDirectoryNolock(dh_t dh) {
  auto itor = created_dhs_.find(dh);
  if (itor == created_dhs_.end()) [[unlikely]] {
    LOG(WARNING) << "directory handle not exist: " << dh;
    return kEmptyDirectoryPtr;
  }
  return itor->second;
}

const DirectoryPtr &DirHandle::GetCreatedDirectory(const std::string &dirname) {
  META_HANDLE_LOCK();
  return GetCreatedDirectoryNolock(dirname);
}

const DirectoryPtr &DirHandle::GetCreatedDirectoryNolock(
    const std::string &dirname) {
  std::string dir_name;
  if (!TransformPath(dirname, dir_name)) {
    LOG(ERROR) << "directory path transform failed: " << dirname;
    return kEmptyDirectoryPtr;
  }
  auto itor = created_dirs_.find(dir_name);
  if (unlikely(itor == created_dirs_.end())) {
    LOG(WARNING) << "directory not exist: " << dir_name;
    return kEmptyDirectoryPtr;
  }
  return itor->second;
}

/**
 * 删除带挂载目录的文件夹路径
 *
 * @param dirname UXDB-UDISK约定: DB删除的绝对目录
 *
 * @return 0 success or -1 failed
 */
int32_t DirHandle::DeleteDirectory(dh_t dh) {
  META_HANDLE_LOCK();
  return DeleteDirectoryNolock(dh);
}
int32_t DirHandle::DeleteDirectoryNolock(dh_t dh) {
  LOG(INFO) << "delete dir handle: " << dh;
  const DirectoryPtr dir = FileSystem::Instance()->dir_handle()->GetCreatedDirectoryNolock(dh);
  if (!dir) {
    block_fs_set_errno(ENOENT);
    return -1;
  }

  time_t time = ::time(nullptr);
  dir->set_mtime(time);
  dir->set_ctime(time);

  // 文件夹下面还有文件或者文件夹不能被删除
  if (dir->ChildCount() > 0) {
    LOG(ERROR) << "directory not empty: " << dir->dir_name();
    block_fs_set_errno(ENOTEMPTY | EEXIST);
    return -1;
  }

  if (dir->LinkCount() > 0) {
    LOG(ERROR) << "directory has been opened: " << dir->dir_name();
    block_fs_set_errno(EBUSY);
    return -1;
  }

  // 持久化删除文件夹元数据
  if (!dir->SuicideNolock()) {
    // TODO:失败处理
    return -1;
  }

  // 删除内存文件夹
  std::string parent_dir_name = GetParentDirName(dir->dir_name());
  auto itor = created_dirs_.find(parent_dir_name);
  if (unlikely(itor == created_dirs_.end())) {
    LOG(ERROR) << "parent directory not exist: " << parent_dir_name;
    block_fs_set_errno(ENOENT);
    return -1;
  }
  const DirectoryPtr &parent_dir = itor->second;
  if (!parent_dir->RemovChildDirectory(dir)) {
    return -1;
  }
  if (!RemoveDirectoryFromCreateNolock(dir)) {
    return -1;
  }

  LOG(INFO) << "remove directory success, dh: " << dh;
  block_fs_set_errno(0);
  return 0;
}

/**
 * 创建带挂载目录的文件夹路径
 *
 * \param dirname UXDB-UDISK约定: DB创建的绝对目录
 *
 * \return 0 success or -1 failed
 */
int32_t DirHandle::CreateDirectory(const std::string &path) {
  if (!CheckFileExist(path)) {
    return -1;
  }
  // 校验路径的合法性并且添加分隔符
  std::string dir_name;
  if (!TransformPath(path, dir_name)) {
    return -1;
  }

  // 申请新的文件夹元数据
  std::pair<DirectoryPtr, DirectoryPtr> dirs;
  if (!NewDirectory(dir_name, &dirs)) {
    return -1;
  }
  const DirectoryPtr &parent_dir = dirs.first;
  const DirectoryPtr &curr_dir = dirs.second;

  {
    META_HANDLE_LOCK();
    // 持久化文件夹元数据
    int retry_count = 0;
    while (!curr_dir->WriteMeta()) {
      if (++retry_count >= 3) {
        // 重试3次后仍失败，返回错误
        return -1;
      }
      // 可以在这里添加一些延时，避免立即重试
      // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  // 添加到内存文件夹
  if (!AddDirectory(parent_dir, curr_dir)) {
    return -1;
  }

  LOG(INFO) << "make directory success: " << path;

  errno = 0;
  return 0;
}

/**
 * 删除带挂载目录的文件夹路径
 *
 * \param dirname UXDB-UDISK约定: DB删除的绝对目录
 *
 * \return strip success or failed
 */
int32_t DirHandle::DeleteDirectory(const std::string &path, bool recursive) {
  if (!CheckFileExist(path, false)) {
    return -1;
  }

  // 校验路径的合法性并且添加分隔符
  std::string dir_name;
  if (!TransformPath(path, dir_name)) {
    return -1;
  }

  // 挂载目录不能被删除
  if (IsMountPoint(path)) {
    LOG(ERROR) << "mount point cannot be removed: " << path;
    block_fs_set_errno(EPERM | EBUSY);
    return -1;
  }

  // 查找已经创建的文件夹
  std::pair<DirectoryPtr, DirectoryPtr> dirs;
  if (!FindDirectory(dir_name, &dirs)) {
    return -1;
  }
  const DirectoryPtr &parent_dir = dirs.first;
  const DirectoryPtr &curr_dir = dirs.second;

  // 文件夹下面还有文件或者文件夹不能被删除
  if (curr_dir->ChildCount() > 0) {
    LOG(ERROR) << "directory not empty: " << path;
    block_fs_set_errno(ENOTEMPTY | EEXIST);
    return -1;
  }

  if (curr_dir->LinkCount() > 0) {
    LOG(ERROR) << "directory has been opened: " << path;
    block_fs_set_errno(EBUSY);
    return -1;
  }

  // 持久化删除文件夹元数据
  if (!curr_dir->Suicide()) {
    // TODO:失败处理
    return -1;
  }

  // 删除内存文件夹
  if (!RemoveDirectory(parent_dir, curr_dir)) {
    return -1;
  }

  // 同步从节点
  LOG(INFO) << "remove directory success: " << path;
  errno = 0;
  return 0;
}

int32_t DirHandle::RenameDirectory(const std::string &from,
                                   const std::string &to) {
  if (!CheckFileExist(to)) {
    return -1;
  }

  // 挂载目录不能被重命名
  if (IsMountPoint(from) || IsMountPoint(to)) {
    LOG(ERROR) << "mount point cannot be rename, from: " << from
               << " to: " << to;
    block_fs_set_errno(EPERM | EBUSY);
    return -1;
  }

  // 校验路径的合法性并且添加分隔符
  std::string from_dir_name;
  if (!TransformPath(from, from_dir_name)) {
    return -1;
  }
  std::string to_dir_name;
  if (!TransformPath(to, to_dir_name)) {
    return -1;
  }

  // 查找已经创建的文件夹
  std::pair<DirectoryPtr, DirectoryPtr> dirs;
  if (!FindDirectory(from_dir_name, &dirs)) {
    return -1;
  }
  const DirectoryPtr &curr_dir = dirs.second;

  // 持久化文件夹元数据
  if (!curr_dir->rename(to_dir_name)) {
    // TODO:失败处理
    return -1;
  }

  block_fs_set_errno(0);
  return 0;
}

BLOCKFS_DIR *DirHandle::OpenDirectory(const std::string &path) {
  LOG(INFO) << "open directory " << path;
  // 校验路径的合法性并且添加分隔符
  std::string dir_name;
  if (!TransformPath(path, dir_name)) {
    return nullptr;
  }
  DirectoryPtr dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dir_name);
  if (unlikely(!dir)) {
    block_fs_set_errno(ENOTDIR);
    block_fs_set_errno(ENOENT);
    return nullptr;
  }
  int32_t fd = FileSystem::Instance()->fd_handle()->get_fd();
  if (fd < 0) {
    block_fs_set_errno(ENFILE);
    return nullptr;
  }
  BLOCKFS_DIR *d = new BLOCKFS_DIR();
  // TODO
  open_dirs_[fd] = dir;
  d->fd_ = fd;
  d->dir_ = dir;
  d->inited_ = false;
  dir->IncLinkCount();
  return d;
}

// https://download.csdn.net/download/fronteer/4995825
// https://blog.csdn.net/chenleng8306/article/details/100790301
blockfs_dirent *DirHandle::ReadDirectory(BLOCKFS_DIR *d) {
  META_HANDLE_LOCK();
  if (!d->inited_) {
    d->dir_->ScanDir(d->dir_items_);
    d->dir_->UpdateTimeStamp(false, true, true);
    d->inited_ = true;
  }
  if (d->index_ < d->dir_items_.size()) {
    return d->dir_items_[d->index_++];
  }
  return nullptr;
}

int32_t DirHandle::CloseDirectory(BLOCKFS_DIR *d) {
  LOG(INFO) << "close directory name: " << d->dir_->dir_name()
            << " fd: " << d->fd();
  FileSystem::Instance()->fd_handle()->put_fd(d->fd_);
  for (uint32_t i = 0; i < d->dir_items_.size(); ++i) {
    delete d->dir_items_[i];
  }
  d->inited_ = false;
  d->ClearItem();
  d->dir_->DecLinkCount();
  delete d;
  return 0;
}

void DirHandle::Dump(const std::string &path) noexcept {
  const DirectoryPtr &dir = GetCreatedDirectory(path);
  dir->DumpMeta();
}

}