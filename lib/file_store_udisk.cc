#include "file_store_udisk.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>

#include <filesystem>
#include <functional>

#include "config_load.h"
#include "logging.h"

namespace udisk::blockfs {

static const std::string kBlockDevivePath = "/sys/block";

FileSystem::FileSystem() {}

FileSystem::~FileSystem() { UnmountFileSystem(); }

FileSystem *FileSystem::g_instance = new FileSystem();

FileSystem *FileSystem::Instance() { return g_instance; }

const uint64_t FileSystem::GetMaxSupportFileNumber() noexcept {
  return super_meta()->max_file_num;
}

const uint64_t FileSystem::GetMaxSupportBlockNumer() noexcept {
  return super_meta()->max_support_block_num_;
}

const uint64_t FileSystem::GetFreeBlockNumber() noexcept {
  return super_meta()->curr_block_num_;
}

const uint64_t FileSystem::GetBlockSize() noexcept {
  return super_meta()->block_size_;
}

const uint64_t FileSystem::GetFreeFileNumber() noexcept {
  return file_handle()->free_meta_size();
}

const uint64_t FileSystem::GetMaxFileMetaSize() noexcept {
  return super_meta()->file_meta_size_;
}

const uint64_t FileSystem::GetMaxFileNameLength() noexcept {
  return super_meta()->max_file_name_len_;
}

/**
 * mount the blockfs filesystem
 *
 * \param moun_info
 *
 * \return 0 on success, -1 on error
 */
int32_t FileSystem::MountFileSystem(const std::string& config_path) {
  if (config_path.empty()) [[unlikely]] {
    LOG(ERROR) << "config path is empty";
    return -1;
  }

  ConfigLoader loader = ConfigLoader(config_path);
  if (!loader.ParseConfig(&mount_config_)) {
    return -1;
  }

  if (!Initialize()) {
    return -1;
  }

  if (!OpenTarget(mount_config_.device_uuid_)) {
    return -1;
  }
  if (!shm_manager_->Initialize(true)) {
    return -1;
  }
  if (!InitializeMeta()) {
    return -1;
  }
  super()->set_uxdb_mount_point(mount_config_.uxdb_mount_point_);
  if (MakeMountPoint(mount_config_.uxdb_mount_point_) < 0) {
    return -1;
  }
  return 0;
}

/**
 * remount the blockfs filesystem
 *
 * \param void
 *
 * \return success is zero, otherwise -1
 */
int32_t FileSystem::RemountFileSystem() {
  remount_ = true;
  Destroy();

  if (!Initialize()) {
    return -1;
  }

  remount_ = false;

  if (!shm_manager_->Initialize(false)) {
    return -1;
  }
  if (!InitializeMeta()) {
    return -1;
  }
  return MakeMountPoint(super()->uxdb_mount_point());
}

/**
 * unmount the blockfs filesystem
 * need to wait all pending io or meta done
 *
 * \param moun_info
 *
 * \return success is zero, otherwise -1
 */
int32_t FileSystem::UnmountFileSystem() {
  Destroy();
  return 0;
}

/**
 * \param dirname: abosolute dirname
 */
int32_t FileSystem::CreateDirectory(const std::string& path) {
  LOG(INFO) << "make directory: " << path;
  return dir_handle()->CreateDirectory(path);
}

int32_t FileSystem::NewDirectory(const std::string& dirname,
                                std::unique_ptr<Directory>* result) {
  return 0;
}

// List Directory
int32_t FileSystem::ListDirectory(const std::string& path, FileInfo** filelist,
                                 int* num) {
  return 0;
}

/**
 * Delete Directory
 *
 * \param dirname: abosolute dirname
 *
 * \return success or failed
 */
int32_t FileSystem::DeleteDirectory(const std::string& path, bool recursive) {
  LOG(INFO) << "remove directory: " << path;
  return dir_handle()->DeleteDirectory(path, recursive);
}

// Lock Directory
int32_t FileSystem::LockDirectory(const std::string& path) { return 0; }
// Unlock Directory
int32_t FileSystem::UnlockDirectory(const std::string& path) { return 0; }

/**
 * open a directory
 *
 * \param dirname: abosolute dirname
 *
 * \return success or failed
 */
BLOCKFS_DIR* FileSystem::OpenDirectory(const std::string& dirname) {
  LOG(INFO) << "open directory: " << dirname;
  return dir_handle()->OpenDirectory(dirname);
}

/**
 * read a directory
 *
 * \param dir: dir info struct
 *
 * \return success or failed
 */
block_fs_dirent* FileSystem::ReadDirectory(BLOCKFS_DIR* dir) {
  return dir_handle()->ReadDirectory(dir);
}

int32_t FileSystem::ReadDirectoryR(BLOCKFS_DIR* dir, block_fs_dirent* entry,
                                  block_fs_dirent** result) {
  return dir_handle()->ReadDirectoryR(dir, entry, result);
  ;
}

/**
 * close a directory
 *
 * \param dir: dir info struct
 *
 * \return success or failed
 */
int32_t FileSystem::CloseDirectory(BLOCKFS_DIR* dir) {
  return dir_handle()->CloseDirectory(dir);
}

/**
 * stat a file or directory
 *
 * \param path: abosolute path
 * \param buf: struct stat buffer
 *
 * \return success or failed
 */
int32_t FileSystem::StatPath(const std::string& path, struct stat* buf) {
  LOG(INFO) << "stat path: " << path;
  if (unlikely(path.empty())) {
    LOG(ERROR) << "stat path empty";
    errno = EINVAL;
    return -1;
  }
  LOG(INFO) << "stat path: " << path;
  std::string path_name = path;
  // 如果是带尾部分隔符,只需要判断文件夹
  // 挂载目录检查可能不带/, 所以要优先判断
  if (super()->is_mount_point(path_name) ||
      path_name[path_name.size() - 1] == '/') {
    DirectoryPtr dir = dir_handle()->GetCreatedDirectory(path_name);
    if (!dir) {
      errno = ENOENT;
      return -1;
    }
    dir->stat(buf);
  } else {
    FilePtr file = file_handle()->GetCreatedFile(path_name);
    if (!file) {
      DirectoryPtr dir = dir_handle()->GetCreatedDirectory(path_name);
      if (!dir) {
        errno = ENOENT;
        return -1;
      }
      dir->stat(buf);
    } else {
      file->stat(buf);
    }
  }
  errno = 0;
  return 0;
}

int32_t FileSystem::StatPath(const int32_t fd, struct stat* fileinfo) {
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    DirectoryPtr dir = dir_handle()->GetOpenDirectory(fd);
    if (!dir) {
      errno = ENOTDIR;
      errno = ENOENT;
      return -1;
    }
    dir->stat(fileinfo);
  } else {
    open_file->file()->stat(fileinfo);
  }
  return 0;
}

int32_t FileSystem::StatVFS(const std::string& path,
                           struct statvfs* buf) {
  // f_bsize: 文件系统块大小
  buf->f_bsize = GetBlockSize();
  // f_frsize: 文件系统的片段大小，单位是字节
  buf->f_frsize = 512;
  // f_blocks: 文件系统数据块总数
  buf->f_blocks = GetMaxSupportBlockNumer();
  // f_bfree: 可用块数
  buf->f_bfree = GetFreeBlockNumber();
  buf->f_bavail = buf->f_bfree;
  // f_files: 文件结点总数
  buf->f_files = GetMaxSupportFileNumber();
  // f_ffree: 可用文件结点数
  buf->f_ffree = GetFreeFileNumber();
  buf->f_favail = buf->f_ffree;

  // f_fsid: 文件系统标识 ID
  buf->f_fsid = 0;
  // f_namemax: 最大文件长度
  buf->f_namemax = GetMaxFileNameLength();
  return 0;
}

int32_t FileSystem::StatVFS(const int32_t fd, struct statvfs* buf) {
  // f_bsize: 文件系统块大小
  buf->f_bsize = GetBlockSize();
  // f_frsize: 分栈大小
  buf->f_frsize = 0;
  // f_blocks: 文件系统数据块总数
  buf->f_blocks = GetMaxSupportBlockNumer();
  // f_bfree: 可用块数
  buf->f_bavail = GetFreeBlockNumber();
  // f_bavail:非超级用户可获取的块数
  buf->f_blocks = GetMaxSupportBlockNumer();
  // f_files: 文件结点总数
  buf->f_files = GetMaxSupportFileNumber();
  // f_ffree: 可用文件结点数
  buf->f_ffree = GetFreeFileNumber();
  // f_favail: 非超级用户的可用文件结点数
  buf->f_favail = GetMaxSupportFileNumber();

  // f_fsid: 文件系统标识 ID
  buf->f_fsid = 0;
  // f_flag: 挂载标记
  buf->f_flag = 0;
  // f_namemax: 最大文件长度
  buf->f_namemax = GetMaxFileNameLength();
  return 0;
}

// GetFileSize: get real file size
int32_t FileSystem::GetFileSize(const std::string& path, int64_t* file_size) {
  return 0;
}

int32_t FileSystem::OpenFile(const std::string& path, int32_t flags,
                            int32_t mode) {
  if (path.empty()) [[unlikely]] {
    errno = EINVAL;
    return -1;
  }
  LOG(INFO) << "open file: " << path;
  return file_handle()->open(path, flags, mode);
}

int32_t FileSystem::CloseFile(int32_t fd) {
  LOG(INFO) << "close file fd: " << fd;
  return file_handle()->close(fd);
}

int32_t FileSystem::FileExists(const std::string& path) { return 0; }

int32_t FileSystem::CreateFile(const std::string& path, mode_t mode) {
  // O_TMPFILE
  return file_handle()->CreateFile(path, mode) ? 0 : -1;
}

int32_t FileSystem::DeleteFile(const std::string& path) {
  return file_handle()->unlink(path);
}

/**
 * rename a file name or directory name
 *
 * \param oldpath: abosolute oldpath
 * \param newpath: abosolute newpath
 *
 * \return success or failed
 */
int32_t FileSystem::RenamePath(const std::string& oldpath,
                              const std::string& newpath) {
  LOG(INFO) << "old path: " << oldpath << " newpath: " << newpath;
  if (unlikely(oldpath.empty() || newpath.empty())) {
    LOG(ERROR) << "rename path error, oldpath: " << oldpath
               << " newpath: " << newpath;
    errno = EINVAL;
    return -1;
  }

  FilePtr dest_file = file_handle()->GetCreatedFile(newpath);
  if (dest_file) {
    LOG(ERROR) << "new file name exists, file_name: " << newpath;
    return -1;
  }

  FilePtr file = file_handle()->GetCreatedFile(oldpath);
  if (!file) {
    DirectoryPtr dir = dir_handle()->GetCreatedDirectory(oldpath);
    if (!dir) {
      errno = ENOENT;
      return -1;
    }
    return dir_handle()->RenameDirectory(oldpath, newpath);
  }
  // TODO: 如果是dh也需要改变的话
  return file->rename(newpath);
}

int32_t FileSystem::TruncateFile(const std::string& filename, int64_t size) {
  if (size < 0) [[unlikely]] {
    block_fs_set_errno(EINVAL);
    return -1;
  }
  LOG(INFO) << "truncate file: " << filename << " len: " << size;
  FilePtr file = file_handle()->GetCreatedFile(filename);
  if (!file) {
    errno = ENOENT;
    return -1;
  }
  return file->ftruncate(size);
}

int32_t FileSystem::TruncateFile(const int32_t fd, int64_t size) {
  if (unlikely(size < 0)) {
    block_fs_set_errno(EINVAL);
    return -1;
  }
  LOG(INFO) << "truncate file fd: " << fd << " len: " << size;
  // 两个操作在一个锁范围
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->file()->ftruncate(size);
}

int32_t FileSystem::PosixFallocate(int32_t fd, int64_t offset, int64_t len) {
  LOG(INFO) << "posix fallocate file fd: " << fd << " offset: " << offset
            << " len: " << len;
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->file()->posix_fallocate(offset, len);
}

int64_t FileSystem::ReadFile(int32_t fd, void* buf, size_t len) {
  LOG(INFO) << "read file fd: " << fd << " len: " << len;
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->read(buf, len, open_file->append_pos());
}

int64_t FileSystem::WriteFile(int32_t fd, const void* buf, size_t len) {
  LOG(INFO) << "write file fd: " << fd;
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->write(buf, len, open_file->append_pos());
}

int64_t FileSystem::PreadFile(ino_t fd, void* buf, size_t len, off_t offset) {
  LOG(INFO) << "pread file fd: " << fd << " len: " << len << " offset: "
            << offset;
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->pread(buf, len, offset);
}

int64_t FileSystem::PwriteFile(ino_t fd, const void* buf, size_t len,
                              off_t offset) {
  LOG(INFO) << "pwrite file fd: " << fd << " len: " << len << " offset: "
            << offset;
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->pwrite(buf, len, offset);
}

off_t FileSystem::SeekFile(ino_t fd, off_t offset, int whence) {
  LOG(INFO) << "lseek file fd: " << fd << " offset: " << offset;
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->lseek(offset, whence);
}

int32_t FileSystem::FcntlFile(int32_t fd, int32_t set_flag) {
  OpenFilePtr open_file = FileSystem::Instance()->file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  open_file->set_open_flags(set_flag);
  return 0;
}

int32_t FileSystem::FcntlFile(int32_t fd, int16_t lock_type) {
  OpenFilePtr open_file = FileSystem::Instance()->file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  switch (lock_type) {
    case F_UNLCK:
      if (!open_file->file()->locked()) {
        errno = ENOLCK;
        return -1;
      }
      open_file->file()->set_locked(false);
      break;
    case F_RDLCK:
      if (open_file->file()->locked()) {
        errno = EACCES;
        return -1;
      }
      open_file->file()->set_locked(true);
      break;
    case F_WRLCK:
      if (open_file->file()->locked()) {
        errno = EACCES;
        return -1;
      }
      open_file->file()->set_locked(true);
      break;
    default:
      return -1;
  }
  return 0;
}

int32_t FileSystem::Sync() {
  LOG(INFO) << "sync file system, persistent timestamp only";
  return file_handle()->UpdateMeta() ? 0 : -1;
}

int32_t FileSystem::FileSync(const int32_t fd) {
  LOG(INFO) << "fsync file fd: " << fd;
  return file_handle()->fsync(fd);
}

int32_t FileSystem::FileDataSync(const int32_t fd) {
  LOG(INFO) << "fdatasync file fd: " << fd;
  return file_handle()->fsync(fd);
}

int32_t FileSystem::FileDup(const int32_t fd) { return file_handle()->dup(fd); }

// https://blog.csdn.net/linlin2178/article/details/57412568?utm_medium=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase

/**
 * remove a file or directory
 *
 * \param path: abosolute path
 * https://blog.csdn.net/qinrenzhi/article/details/94043485
 * if path is a directory, call rmdir()
 * if path is a file, call unlink()
 *
 * \return success or failed
 */
int32_t FileSystem::RemovePath(const std::string& path) {
  LOG(INFO) << "remove path: " << path;
  if (unlikely(path.empty())) {
    LOG(ERROR) << "remove file path empty";
    errno = EINVAL;
    return -1;
  }
  FilePtr file = file_handle()->GetCreatedFile(path);
  if (file) {
    return file_handle()->unlink(path);
  } else {
    // errno = ENOENT;
    return dir_handle()->DeleteDirectory(path, false);
  }
}

/**
 * Initialize handle in order
 *
 * \return success or failed
 */
bool FileSystem::Initialize() {
  if (!remount_) {
    device_ = new BlockDevice();
    shm_manager_ = new ShmManager();
  }
  handle_vector_[kSuperBlockHandle] = new SuperBlock();
  handle_vector_[kFDHandle] = new FdHandle();
  handle_vector_[kBlockHandle] = new BlockHandle();
  handle_vector_[kDirectoryHandle] = new DirHandle();
  handle_vector_[kFileHandle] = new FileHandle();
  handle_vector_[kFileBlockHandle] = new FileBlockHandle();
  return true;
}

/**
 * delete handle, close device, delete lock
 *
 * \param void
 *
 * \return void
 */
void FileSystem::Destroy() {
  LOG(DEBUG) << "close file store now";
  for (auto& handle : handle_vector_) {
    if (handle) {
      delete handle;
      handle = nullptr;
    }
  }

  if (!remount_) {
    if (shm_manager_) {
      delete shm_manager_;
      shm_manager_ = nullptr;
    }
    if (device_) {
      delete device_;
      device_ = nullptr;
    }
  }
}

/**
 * format seperate fs metadata
 */
bool FileSystem::FormatFSMeta() {
  for (auto& handle : handle_vector_) {
    if (!handle->FormatAllMeta()) {
      return false;
    }
  }
  return true;
}

/**
 * zero fs data
 */
bool FileSystem::FormatFSData() {
  int64_t ret;
  const uint64_t zero_buffer_len = 4 * M;
  AlignBufferPtr buffer =
      std::make_shared<AlignBuffer>(zero_buffer_len, dev()->block_size());

  uint64_t zero_data_offset = super_meta()->block_data_start_offset_;
  uint64_t zero_data_len = zero_buffer_len;
  while (true) {
    if (unlikely(super_meta()->curr_udisk_size_ <
                 (zero_data_offset + zero_buffer_len))) {
      zero_data_len = super_meta()->curr_udisk_size_ - zero_data_offset;
    }
    ret = dev()->PwriteDirect(buffer->data(), zero_data_len, zero_data_offset);
    if (ret != static_cast<int64_t>(zero_data_len)) {
      LOG(ERROR) << "write zero data error size:" << ret;
      return false;
    }
    zero_data_offset += zero_data_len;
    if (unlikely(zero_data_offset == super_meta()->curr_udisk_size_)) {
      break;
    }
  }
  LOG(INFO) << "format data zero success";
  return true;
}

/**
 * format raw block device using blockfs
 *
 * @param dev_name
 *
 * @return success or failed
 */
bool FileSystem::Format(const std::string& dev_name) {
  if (!Initialize()) {
    return false;
  }

  if (!device_->Open(dev_name)) {
    return false;
  }
  ShmManager::CleanupDirtyShareMemory();

  if (!FormatFSMeta()) {
    return false;
  }
  // return FormatFSData();
  return true;
}

/**
 * verify blockfs meta
 *
 * \param dev_name
 *
 * \return success or failed
 */
bool FileSystem::Check(const std::string& dev_name, const std::string &log_level) {
  Logger::set_min_level(Logger::LogLevelConvert(log_level));
  if (!Initialize()) {
    return false;
  }
  if (!device_->Open(dev_name)) {
    return false;
  }
  if (!shm_manager_->Initialize()) {
    return false;
  }
  return InitializeMeta();
}

/*
 * Ref: https://github.com/karelzak/util-linux/blob/is_master/misc-utils/lsblk.c
 * Reads root nodes (devices) from /sys/block into devices tree
 *
 * Other way: lsblk | grep -v NAME | grep -v vda | awk '{print $1}'| xargs
 */
int EasyReaddir(const std::string& dir, std::set<std::string>* out) {
  DIR* h = ::opendir(dir.c_str());
  if (!h) {
    return -errno;
  }
  struct dirent* de = nullptr;
  while ((de = ::readdir(h))) {
    if (::strcmp(de->d_name, ".") == 0 || ::strcmp(de->d_name, "..") == 0) {
      continue;
    }
    out->insert(de->d_name);
  }
  ::closedir(h);
  return 0;
}

bool FileSystem::OpenTarget(const std::string& uuid) {
  try {
    for (const auto& entry : std::filesystem::directory_iterator(kBlockDevivePath)) {
      if (!device_->Open("/dev/" + entry.path().filename().string())) {
        continue;
      }
      SuperBlockMeta meta;
      bool success = ShmManager::PrefetchSuperMeta(meta);
      std::string meta_uuid = meta.uuid_;
      if (!success || meta_uuid != uuid) {
        LOG(DEBUG) << "read meta error or uuid mismatch,"
                   << " read uuid: " << meta_uuid << " wanted uuid: " << uuid;
        device_->Close();
        continue;
      }
      return true;
    }
  } catch (std::filesystem::filesystem_error& e) {
    LOG(ERROR) << "failed to open dir:" << kBlockDevivePath
               << ", error: " << e.what();
  }
  return false;
}

bool FileSystem::InitializeMeta() {
  for (auto& handle : handle_vector_) {
    if (!handle->InitializeMeta()) {
      return false;
    }
  }
  return true;
}

/**
 * create mount point
 *
 * \param mount_point
 *
 * \return true or false
 */
int FileSystem::MakeMountPoint(const char* mount_point) {
  LOG(INFO) << "create fs mount point: " << mount_point;
  if (!dir_handle()->GetCreatedDirectory(mount_point)) {
    return CreateDirectory(mount_point);
  }
  return 0;
}

int FileSystem::MakeMountPoint(const std::string& mount_point) {
  LOG(INFO) << "create fs mount point: " << mount_point;
  if (!dir_handle()->GetCreatedDirectory(mount_point)) {
    return CreateDirectory(mount_point);
  }
  return 0;
}

void FileSystem::DumpFileMeta(const std::string& path) {
  LOG(INFO) << "dump file meta: " << path;
  std::string path_name = path;
  // 如果是带尾部分隔符,只需要判断文件夹
  // 挂载目录检查可能不带/, 所以要优先判断
  if (super()->is_mount_point(path_name) ||
      path_name[path_name.size() - 1] == '/') {
    return;
  } else {
    FilePtr file = file_handle()->GetCreatedFile(path_name);
    if (!file) {
      return;
    }
    file->DumpMeta();
  }
}

}