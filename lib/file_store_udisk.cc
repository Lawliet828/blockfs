#include "file_store_udisk.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>

#include <functional>

#include "config_load.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

static const std::string kBlockDevivePath = "/sys/block";

FileStore::FileStore() {}

FileStore::~FileStore() { UnmountFileSystem(); }

FileStore *FileStore::g_instance = new FileStore();

FileStore *FileStore::Instance() { return g_instance; }

const uint64_t FileStore::GetMaxSupportFileNumber() noexcept {
  return super_meta()->max_support_file_num_;
}

const uint64_t FileStore::GetMaxSupportDirectoryNumber() noexcept {
  return super_meta()->max_support_file_num_;
}

const uint64_t FileStore::GetMaxSupportBlockNumer() noexcept {
  return super_meta()->max_support_block_num_;
}

const uint64_t FileStore::GetMaxBlockNumber() noexcept {
  return super_meta()->max_support_block_num_;
}

const uint64_t FileStore::GetFreeBlockNumber() noexcept {
  return super_meta()->curr_block_num_;
}

const uint64_t FileStore::GetBlockSize() noexcept {
  return super_meta()->block_size_;
}

const uint64_t FileStore::GetFreeFileNumber() noexcept {
  return file_handle()->free_meta_size();
}

const uint64_t FileStore::GetMaxFileMetaSize() noexcept {
  return super_meta()->file_meta_size_;
}

const uint64_t FileStore::GetMaxFileNameLength() noexcept {
  return super_meta()->max_file_name_len_;
}

/**
 * mount the blockfs filesystem
 *
 * \param moun_info
 *
 * \return 0 on success, -1 on error
 */
int32_t FileStore::MountFileSystem(const std::string& config_path,
                                   bool is_master) {
  if (unlikely(config_path.empty())) {
    LOG(ERROR) << "config path is empty";
    return -1;
  }

  FILE_STORE_LOCK();

  ConfigLoader loader = ConfigLoader(config_path);
  if (!loader.ParseConfig(&mount_config_)) {
    return -1;
  }

  LOG(DEBUG) << "run as master node: " << is_master;
  mount_stat_.set_is_master(is_master);

  if (!Initialize()) {
    return -1;
  }

  if (!OpenTarget(mount_config_.device_uuid_)) {
    return -1;
  }
  if (!shm_manager_->Initialize(true)) {
    return -1;
  }
  // super()->set_uxdb_mount_point(info.mount_point_);
  if (!InitializeMeta(true)) {
    return -1;
  }
  super()->set_uxdb_mount_point(mount_config_.uxdb_mount_point_);
  if (MakeMountPoint(mount_config_.uxdb_mount_point_) < 0) {
    return -1;
  }
  return 0;
}

int32_t FileStore::MountFileLock() {
  lock_ = new PosixFileLock(mount_config_.device_uuid_);
  if (!lock_ || !lock_->lock(true)) {
    // bfs mounted by tool, but not set master
    // The master node mount is expected to be successful
    if (mount_stat_.is_master_) {
      return -1;
    }
  }
  return 0;
}

/**
 * mount the blockfs filesystem
 *
 * \param uuid, size
 *
 * \return success is zero, otherwise -1
 */
int32_t FileStore::MountGrowfs(uint64_t size) {
  LOG(ERROR) << "block fs extend udisk not supported yet";
  return 0;
}

/**
 * remount the blockfs filesystem
 *
 * \param void
 *
 * \return success is zero, otherwise -1
 */
int32_t FileStore::RemountFileSystem() {
  FILE_STORE_LOCK();
  remount_ = true;
  Destroy();

  if (!Initialize()) {
    return -1;
  }

  remount_ = false;

  if (!shm_manager_->Initialize(false)) {
    return -1;
  }
  if (!InitializeMeta(true)) {
    return -1;
  }
  set_is_mounted(true);
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
int32_t FileStore::UnmountFileSystem() {
  FILE_STORE_LOCK();
  Destroy();
  return 0;
}

/**
 * Create directory
 *
 * \param dirname: abosolute dirname
 *
 * \return success or failed
 */
int32_t FileStore::CreateDirectory(const std::string& path) {
  LOG(INFO) << "make directory: " << path;
  if (!is_master()) {
    LOG(WARNING) << "mock return success, dirname: " << path;
    return 0;
  }
  if (!CheckPermission("mkdir", path.c_str())) {
    return -1;
  }
  return dir_handle()->CreateDirectory(path);
}

int32_t FileStore::NewDirectory(const std::string& dirname,
                                std::unique_ptr<Directory>* result) {
  return 0;
}

// List Directory
int32_t FileStore::ListDirectory(const std::string& path, FileInfo** filelist,
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
int32_t FileStore::DeleteDirectory(const std::string& path, bool recursive) {
  LOG(INFO) << "remove directory: " << path;
  if (!is_master()) {
    LOG(WARNING) << "mock return success, dirname: " << path;
    return 0;
  }
  if (!CheckPermission("rmdir", path.c_str())) {
    return -1;
  }
  return dir_handle()->DeleteDirectory(path, recursive);
}

// Lock Directory
int32_t FileStore::LockDirectory(const std::string& path) { return 0; }
// Unlock Directory
int32_t FileStore::UnlockDirectory(const std::string& path) { return 0; }

/**
 * open a directory
 *
 * \param dirname: abosolute dirname
 *
 * \return success or failed
 */
BLOCKFS_DIR* FileStore::OpenDirectory(const std::string& dirname) {
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
block_fs_dirent* FileStore::ReadDirectory(BLOCKFS_DIR* dir) {
  return dir_handle()->ReadDirectory(dir);
}

int32_t FileStore::ReadDirectoryR(BLOCKFS_DIR* dir, block_fs_dirent* entry,
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
int32_t FileStore::CloseDirectory(BLOCKFS_DIR* dir) {
  return dir_handle()->CloseDirectory(dir);
}

int32_t FileStore::ChangeWorkDirectory(const std::string& path) {
  if (!is_mounted()) {
    return -1;
  }
  if (!is_master()) {
    LOG(WARNING) << "mock return success, dirname: " << path;
    return 0;
  }
  LOG(WARNING) << "chdir not implemented yet";
  return 0;
}

int32_t FileStore::GetWorkDirectory(std::string& path) {
  path = "/mnt/mysql/data";
  return 0;
}

// Du
int32_t FileStore::DiskUsage(const std::string& path, int64_t* du_size) {
  return 0;
}
// Access
int32_t FileStore::Access(const std::string& path, int32_t mode) {
  LOG(INFO) << "access path name: " << path;
  if (unlikely(path.empty())) {
    LOG(ERROR) << "stat file path empty";
    errno = EINVAL;
    return -1;
  }
  if (!is_mounted()) {
    return -1;
  }
  if (!is_master()) {
    LOG(WARNING) << "mock return success, path: " << path;
    return 0;
  }
  FilePtr file = file_handle()->GetCreatedFile(path);
  if (!file) {
    DirectoryPtr dir = dir_handle()->GetCreatedDirectory(path);
    if (!dir) {
      errno = ENOENT;
      return -1;
    }
    return dir->access(mode);
  } else {
    return file->access(mode);
  }
}

/**
 * stat a file or directory
 *
 * \param path: abosolute path
 * \param buf: struct stat buffer
 *
 * \return success or failed
 */
int32_t FileStore::StatPath(const std::string& path, struct stat* buf) {
  LOG(INFO) << "stat path: " << path;
  if (unlikely(path.empty())) {
    LOG(ERROR) << "stat path empty";
    errno = EINVAL;
    return -1;
  }
  if (!is_mounted()) {
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

int32_t FileStore::StatPath(const int32_t fd, struct stat* fileinfo) {
  if (!is_mounted()) {
    errno = EBUSY;
    return -1;
  }
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

int32_t FileStore::StatVFS(const std::string& path,
                           struct block_fs_statvfs* buf) {
  buf->f_type = kBlockFsMagic;
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

int32_t FileStore::StatVFS(const int32_t fd, struct block_fs_statvfs* buf) {
  buf->f_type = kBlockFsMagic;
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
int32_t FileStore::GetFileSize(const std::string& path, int64_t* file_size) {
  return 0;
}

// https://www.cnblogs.com/xiaojiang1025/p/5933755.html
int32_t FileStore::OpenFile(const std::string& path, int32_t flags,
                            int32_t mode) {
  if (unlikely(path.empty())) {
    errno = EINVAL;
    return -1;
  }
  LOG(INFO) << "open file: " << path;
  if (!is_mounted()) {
    return -1;
  }
  return file_handle()->open(path, flags, mode);
}

int32_t FileStore::CloseFile(int32_t fd) {
  LOG(INFO) << "close file fd: " << fd;
  if (!is_mounted()) {
    return -1;
  }
  return file_handle()->close(fd);
}

int32_t FileStore::FileExists(const std::string& path) { return 0; }

int32_t FileStore::CreateFile(const std::string& path, mode_t mode) {
  if (!is_master()) {
    LOG(INFO) << "mock return success";
    return 0;
  }
  if (!CheckPermission("create", path.c_str())) {
    return -1;
  }
  // O_TMPFILE
  return file_handle()->CreateFile(path, mode) ? 0 : -1;
}

int32_t FileStore::DeleteFile(const std::string& path) {
  if (!is_master()) {
    LOG(INFO) << "mock return success";
    return 0;
  }
  if (!CheckPermission("unlink", path.c_str())) {
    return -1;
  }
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
int32_t FileStore::RenamePath(const std::string& oldpath,
                              const std::string& newpath) {
  LOG(INFO) << "old path: " << oldpath << " newpath: " << newpath;
  if (unlikely(oldpath.empty() || newpath.empty())) {
    LOG(ERROR) << "rename path error, oldpath: " << oldpath
               << " newpath: " << newpath;
    errno = EINVAL;
    return -1;
  }
  if (!is_master()) {
    LOG(INFO) << "mock rename return success";
    return 0;
  }
  if (!is_mounted()) {
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

// Returns 0 on success.
int32_t FileStore::CopyFile(const std::string& from, const std::string& to) {
  return 0;
}

int32_t FileStore::TruncateFile(const std::string& filename, int64_t size) {
  if (!is_master()) {
    LOG(INFO) << "mock return success";
    return 0;
  }
  if (!CheckPermission("truncate", filename.c_str())) {
    return -1;
  }
  if (unlikely(size < 0)) {
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

int32_t FileStore::TruncateFile(const int32_t fd, int64_t size) {
  if (!is_master()) {
    LOG(WARNING) << "mock return success, fd: " << fd;
    return 0;
  }
  if (!CheckPermission("ftruncate", fd)) {
    return -1;
  }
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

int32_t FileStore::PosixFallocate(int32_t fd, int64_t offset, int64_t len) {
  if (!CheckPermission("posix_fallocate", fd)) {
    return -1;
  }
  LOG(INFO) << "posix fallocate file fd: " << fd << " offset: " << offset
            << " len: " << len;
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->file()->posix_fallocate(offset, len);
}

int32_t FileStore::ChmodPath(const std::string& path, int32_t mode) {
  if (unlikely(path.empty())) {
    LOG(ERROR) << "chmod file path empty";
    errno = EINVAL;
    return -1;
  }
  if (!is_mounted()) {
    errno = EBUSY;
    return -1;
  }
  if (!is_master()) {
    LOG(INFO) << "mock return success";
    return 0;
  }
  FilePtr file = file_handle()->GetCreatedFile(path);
  if (!file) {
    DirectoryPtr dir = dir_handle()->GetCreatedDirectory(path);
    if (!dir) {
      errno = ENOENT;
      return -1;
    }
    return dir->chmod(mode);
  } else {
    return file->chmod(mode);
  }
}

int32_t FileStore::GetFileModificationTime(const std::string& filename,
                                           uint64_t* filemtime) {
  return 0;
}

int64_t FileStore::ReadFile(int32_t fd, void* buf, size_t len) {
  LOG(INFO) << "read file fd: " << fd << " len: " << len;
  if (!is_mounted()) {
    return -1;
  }
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->read(buf, len, open_file->append_pos());
}

int64_t FileStore::WriteFile(int32_t fd, const void* buf, size_t len) {
  LOG(INFO) << "write file fd: " << fd;
  if (!CheckPermission("write", fd)) {
    return -1;
  }
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->write(buf, len, open_file->append_pos());
}

int64_t FileStore::PreadFile(int32_t fd, void* buf, size_t len, off_t offset) {
  if (!is_mounted()) {
    return -1;
  }
  OpenFilePtr file = file_handle()->GetOpenFile(fd);
  if (!file) {
    errno = ENOENT;
    return -1;
  }
  return file->pread(buf, len, offset);
}

int64_t FileStore::PwriteFile(int32_t fd, const void* buf, size_t len,
                              off_t offset) {
  if (!CheckPermission("pwrite", fd)) {
    return -1;
  }
  OpenFilePtr open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->pwrite(buf, len, offset);
}

off_t FileStore::SeekFile(int32_t fd, off_t offset, int whence) {
  LOG(INFO) << "lseek file fd: " << fd << " offset: " << offset;
  const OpenFilePtr& open_file = file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  return open_file->lseek(offset, whence);
}

int32_t FileStore::FcntlFile(int32_t fd, int32_t set_flag) {
  if (unlikely(!is_mounted())) {
    errno = EBUSY;
    return -1;
  }
  OpenFilePtr open_file = FileStore::Instance()->file_handle()->GetOpenFile(fd);
  if (!open_file) {
    // errno = ENOENT;
    return -1;
  }
  open_file->set_open_flags(set_flag);
  return 0;
}

int32_t FileStore::FcntlFile(int32_t fd, int16_t lock_type) {
  if (unlikely(!is_mounted())) {
    errno = EBUSY;
    return -1;
  }
  OpenFilePtr open_file = FileStore::Instance()->file_handle()->GetOpenFile(fd);
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

int32_t FileStore::Sync() {
  LOG(INFO) << "sync file system, persistent timestamp only";
  return file_handle()->UpdateMeta() ? 0 : -1;
}

int32_t FileStore::FileSync(const int32_t fd) {
  LOG(INFO) << "fsync file fd: " << fd;
  if (!is_master()) {
    LOG(WARNING) << "mock return success, fd: " << fd;
    return 0;
  }
  if (!CheckPermission("fsync", fd)) {
    return -1;
  }
  return file_handle()->fsync(fd);
}

int32_t FileStore::FileDataSync(const int32_t fd) {
  LOG(INFO) << "fdatasync file fd: " << fd;
  if (!is_master()) {
    LOG(WARNING) << "mock return success, fd: " << fd;
    return 0;
  }
  if (!CheckPermission("fdatasync", fd)) {
    return -1;
  }
  return file_handle()->fsync(fd);
}

int32_t FileStore::FileDup(const int32_t fd) { return file_handle()->dup(fd); }

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
int32_t FileStore::RemovePath(const std::string& path) {
  LOG(INFO) << "remove path: " << path;
  if (!is_master()) {
    LOG(INFO) << "mock return success";
    return 0;
  }
  if (!is_mounted()) {
    return -1;
  }
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

// Show system status
int32_t FileStore::SysStat(const std::string& stat_name, std::string* result) {
  LOG(WARNING) << "system stat not implemented yet";
  return -1;
}

int32_t FileStore::LockFile(const std::string& fname) { return 0; }
int32_t FileStore::UnlockFile(const std::string& fname) { return 0; }

int32_t FileStore::GetAbsolutePath(const std::string& in_path,
                                   std::string* output_path) {
  return 0;
}

/**
 * Initialize handle in order
 *
 * \param
 *
 * \return success or failed
 */
bool FileStore::Initialize() {
  if (!remount_) {
    ChecksumInit();
    device_ = new BlockDevice();
    shm_manager_ = new ShmManager();
  }
  handle_vector_[kNegotiationHandle] = new Negotiation();
  handle_vector_[kSuperBlockHandle] = new SuperBlock();
  handle_vector_[kJournalHandle] = new JournalHandle();
  handle_vector_[kFDHandle] = new FdHandle();
  handle_vector_[kBlockHandle] = new BlockHandle();
  handle_vector_[kDirectoryHandle] = new DirHandle();
  handle_vector_[kFileHandle] = new FileHandle();
  handle_vector_[kFileBlockHandle] = new FileBlockHandle();
  return SetCoreDump(true);
}

/**
 * delete handle, close device, delete lock
 *
 * \param void
 *
 * \return void
 */
void FileStore::Destroy() {
  LOG(DEBUG) << "close file store now";
  set_is_mounted(false);
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
    if (lock_) {
      delete lock_;
      lock_ = nullptr;
    }
  }
}

/**
 * format seperate fs metadata
 */
bool FileStore::FormatFSMeta() {
  for (auto& handle : handle_vector_) {
    if (!handle->FormatAllMeta()) {
      return false;
    }
  }
  return true;
}

/**
 * zero fs data
 *
 * \param
 *
 * \return success or failed
 */
bool FileStore::FormatFSData() {
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
    // std::cout << "@@@@ Format zero_data_len: " << zero_data_len
    //           << " zero_data_offset: " << zero_data_offset;
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
bool FileStore::Format(const std::string& dev_name) {
  if (!Initialize()) {
    return false;
  }

  mount_stat_.set_is_master(true);
  if (!device_->Open(dev_name)) {
    return false;
  }
  ShmManager::CleanupDirtyShareMemory();

  return FormatFSMeta();
  // return FormatFSData();
}

/**
 * verify blockfs meta
 *
 * \param dev_name
 *
 * \return success or failed
 */
bool FileStore::Check(const std::string& dev_name, const std::string &log_level) {
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
  return InitializeMeta(true);
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

bool FileStore::OpenTarget(const std::string& uuid) {
  DIR* sys_block_dir = nullptr;
  struct dirent* entry = nullptr;

  sys_block_dir = ::opendir(kBlockDevivePath.c_str());
  if (!sys_block_dir) {
    LOG(ERROR) << "failed to open dir:" << kBlockDevivePath
               << ", errno: " << errno;
    return false;
  }

  while ((entry = ::readdir(sys_block_dir))) {
    if (!device_->Open("/dev/" + std::string(entry->d_name))) {
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
    ::closedir(sys_block_dir);
    return true;
  }
  ::closedir(sys_block_dir);
  return false;
}

/**
 * change is_master attribute
 *
 * \param is_master
 *
 * \return true or false
 */
bool FileStore::InitializeMeta(bool dump) {
  for (auto& handle : handle_vector_) {
    if (!handle->InitializeMeta()) {
      return false;
    }
  }
  if (!FileStore::Instance()->journal_handle()->ReplayAllJournal()) {
    return false;
  }
  set_is_mounted(true);
  return true;
}

/**
 * create mount point
 *
 * \param mount_point
 *
 * \return true or false
 */
int FileStore::MakeMountPoint(const char* mount_point) {
  LOG(INFO) << "create fs mount point: " << mount_point;
  if (!dir_handle()->GetCreatedDirectory(mount_point)) {
    return CreateDirectory(mount_point);
  }
  return 0;
}

int FileStore::MakeMountPoint(const std::string& mount_point) {
  LOG(INFO) << "create fs mount point: " << mount_point;
  if (!dir_handle()->GetCreatedDirectory(mount_point)) {
    return CreateDirectory(mount_point);
  }
  return 0;
}

/**
 * remount the blockfs filesystem
 *
 * \param op: posix operation
 * \param path: abosolute path
 *
 * \return success or failed
 */
bool FileStore::CheckPermission(const char* op, const char* path) {
  if (unlikely(!path)) {
    LOG(ERROR) << op << " file path empty";
    block_fs_set_errno(EINVAL);
    return false;
  }
  if (unlikely(!is_mounted())) {
    LOG(ERROR) << "mount has not finshed yet";
    block_fs_set_errno(EBUSY);
    return false;
  }

  if (!is_master()) {
    LOG(ERROR) << "I am not is_master, cannot " << op << " " << path;
    block_fs_set_errno(EPERM | EROFS);
    return false;
  }
  return true;
}

/**
 * remount the blockfs filesystem
 *
 * \param op: posix operation
 * \param path: abosolute path
 *
 * \return success or failed
 */
bool FileStore::CheckPermission(const char* op, const int32_t handle) {
  if (unlikely(!is_mounted())) {
    LOG(ERROR) << "mount has not finshed yet";
    return false;
  }
  if (unlikely(!is_master())) {
    LOG(ERROR) << "I am not is_master, cannot " << op << " " << handle;
    return false;
  }
  return true;
}

void FileStore::DumpFileMeta(const std::string& path) {
  LOG(INFO) << "dump file meta: " << path;
  if (!is_mounted()) {
    return;
  }
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

}  // namespace blockfs
}  // namespace udisk