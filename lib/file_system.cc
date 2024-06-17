#include "file_system.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>

#include <filesystem>
#include <functional>

#include "config_load.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

FileSystem::FileSystem() {}

FileSystem::~FileSystem() { Destroy(); }

FileSystem* FileSystem::g_instance = new FileSystem();

FileSystem* FileSystem::Instance() { return g_instance; }

/**
 * mount the blockfs filesystem
 *
 * \param moun_info
 *
 * \return 0 on success, -1 on error
 */
int32_t FileSystem::MountFileSystem(const std::string& config_path) {
  if (config_path.empty()) [[unlikely]] {
    SPDLOG_ERROR("config path is empty");
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
 * read a directory
 *
 * \param dir: dir info struct
 *
 * \return success or failed
 */
block_fs_dirent* FileSystem::ReadDirectory(BLOCKFS_DIR* dir) {
  return dir_handle()->ReadDirectory(dir);
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
  if (path.empty()) [[unlikely]] {
    SPDLOG_ERROR("stat path empty");
    errno = EINVAL;
    return -1;
  }
  SPDLOG_INFO("stat path: {}", path);
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

int32_t FileSystem::StatVFS(struct statvfs* buf) {
  // f_bsize: 文件系统块大小
  buf->f_bsize = super_meta()->block_size_;
  // f_frsize: 单位字节, df命令中的块大小
  buf->f_frsize = buf->f_bsize;
  // f_blocks: 文件系统数据块总数
  buf->f_blocks = super_meta()->curr_block_num;
  // f_bfree: 可用块数
  buf->f_bfree = block_handle()->GetFreeBlockNum();
  buf->f_bavail = buf->f_bfree;
  // f_files: 文件结点总数
  buf->f_files = super_meta()->max_file_num;
  // f_ffree: 可用文件结点数
  buf->f_ffree = file_handle()->free_meta_size();
  buf->f_favail = buf->f_ffree;

  // f_fsid: 文件系统标识 ID
  buf->f_fsid = 0;
  // f_namemax: 最大文件长度
  buf->f_namemax = super_meta()->max_file_name_len_;
  SPDLOG_DEBUG("statvfs f_bsize: {} f_blocks: {} f_bfree: {} f_files: {} f_ffree: {}",
               buf->f_bsize, buf->f_blocks, buf->f_bfree, buf->f_files, buf->f_ffree);
  return 0;
}

int32_t FileSystem::CreateFile(const std::string& path, mode_t mode) {
  // O_TMPFILE
  return file_handle()->CreateFile(path, mode) ? 0 : -1;
}

/**
 * rename a file name or directory name
 *
 * \param oldpath: abosolute oldpath
 * \param newpath: abosolute newpath
 */
int32_t FileSystem::RenamePath(const std::string& oldpath,
                               const std::string& newpath) {
  SPDLOG_INFO("rename path: {} -> {}", oldpath, newpath);
  if (oldpath.empty() || newpath.empty()) [[unlikely]] {
    SPDLOG_ERROR("rename path error, oldpath: {} newpath: {}", oldpath, newpath);
    errno = EINVAL;
    return -1;
  }

  FilePtr dest_file = file_handle()->GetCreatedFile(newpath);
  if (dest_file) {
    SPDLOG_ERROR("new file name exists, file_name: {}", newpath);
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
  SPDLOG_INFO("truncate file: {} len: {}", filename, size);
  FilePtr file = file_handle()->GetCreatedFile(filename);
  if (!file) {
    errno = ENOENT;
    return -1;
  }
  return file->ftruncate(size);
}

int32_t FileSystem::TruncateFile(const int32_t fd, int64_t size) {
  SPDLOG_INFO("truncate file fd: {} len: {}", fd, size);
  // 两个操作在一个锁范围
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) [[unlikely]] {
    errno = ENOENT;
    return -1;
  }
  return open_file->file()->ftruncate(size);
}

int64_t FileSystem::ReadFile(int32_t fd, void* buf, size_t len) {
  SPDLOG_INFO("read file fd: {} len: {}", fd, len);
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->read(buf, len, open_file->append_pos());
}

int64_t FileSystem::PreadFile(ino_t fd, void* buf, size_t len, off_t offset) {
  SPDLOG_INFO("pread file fd: {} len: {} offset: {}", fd, len, offset);
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->pread(buf, len, offset);
}

int64_t FileSystem::PwriteFile(ino_t fd, const void* buf, size_t len,
                               off_t offset) {
  SPDLOG_INFO("pwrite file fd: {} len: {} offset: {}", fd, len, offset);
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->pwrite(buf, len, offset);
}

off_t FileSystem::SeekFile(ino_t fd, off_t offset, int whence) {
  SPDLOG_INFO("lseek file fd: {} offset: {}", fd, offset);
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->lseek(offset, whence);
}

int32_t FileSystem::FcntlFile(int32_t fd, int16_t lock_type) {
  OpenFilePtr open_file =
      FileSystem::Instance()->file_handle()->GetOpenFile(fd);
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

/**
 * Initialize handle in order
 *
 * \return success or failed
 */
bool FileSystem::Initialize() {
  if (!remount_) {
    device_ = new Device();
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
  SPDLOG_DEBUG("close file store now");
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
    if (unlikely(super_meta()->device_size <
                 (zero_data_offset + zero_buffer_len))) {
      zero_data_len = super_meta()->device_size - zero_data_offset;
    }
    ret = dev()->PwriteDirect(buffer->data(), zero_data_len, zero_data_offset);
    if (ret != static_cast<int64_t>(zero_data_len)) {
      SPDLOG_ERROR("write zero data error size: {}", ret);
      return false;
    }
    zero_data_offset += zero_data_len;
    if (unlikely(zero_data_offset == super_meta()->device_size)) {
      break;
    }
  }
  SPDLOG_INFO("format data zero success");
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
bool FileSystem::Check(const std::string& dev_name,
                       const std::string& log_level) {
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
  const std::string kBlockDevivePath = "/sys/block";
  try {
    for (const auto& entry :
         std::filesystem::directory_iterator(kBlockDevivePath)) {
      if (!device_->Open("/dev/" + entry.path().filename().string())) {
        continue;
      }
      SuperBlockMeta meta;
      bool success = ShmManager::PrefetchSuperMeta(meta);
      std::string meta_uuid = meta.uuid_;
      if (!success || meta_uuid != uuid) {
        SPDLOG_DEBUG("read meta error or uuid mismatch, read uuid: {} wanted uuid: {}",
                     meta_uuid, uuid);
        device_->Close();
        continue;
      }
      return true;
    }
  } catch (std::filesystem::filesystem_error& e) {
    SPDLOG_ERROR("failed to open dir: {} error: {}", kBlockDevivePath, e.what());
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
 */
int FileSystem::MakeMountPoint(const std::string& mount_point) {
  SPDLOG_INFO("create fs mount point: {}", mount_point);
  if (!dir_handle()->GetCreatedDirectory(mount_point)) {
    return dir_handle()->CreateDirectory(mount_point);
  }
  SPDLOG_INFO("mount point exists: {}", mount_point);
  return 0;
}

void FileSystem::DumpFileMeta(const std::string& path) {
  SPDLOG_INFO("dump file meta: {}", path);
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

}  // namespace udisk::blockfs