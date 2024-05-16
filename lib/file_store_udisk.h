// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_STORE_H_
#define LIB_FILE_STORE_H_

#include <sys/statvfs.h>

#include "aligned_buffer.h"
#include "block_fs_internal.h"
#include "bfs_fuse.h"
#include "block_handle.h"
#include "dir_handle.h"
#include "fd_handle.h"
#include "file_block_handle.h"
#include "file_system.h"
#include "shm_manager.h"
#include "super_block.h"

namespace udisk {
namespace blockfs {

enum MetaHandleType {
  kSuperBlockHandle,
  kFDHandle,
  kBlockHandle,
  kDirectoryHandle,
  kFileHandle,
  kFileBlockHandle,
  kMetaHandleSize,
};

class FileStore : public FileSystem {
 private:
  std::mutex mutex_;
  bool remount_ = false;

  BlockDevice *device_;
  ShmManager *shm_manager_;

  MetaHandle *handle_vector_[kMetaHandleSize];

  static FileStore *g_instance;

 private:
#define FILE_STORE_LOCK() std::lock_guard<std::mutex> lock(mutex_)
  bool FormatFSMeta();
  bool FormatFSData();
  bool Initialize();
  void Destroy();

  bool OpenTarget(const std::string &uuid);
  bool InitializeMeta(bool dump);
  bool Upgrade();
  int MakeMountPoint(const char *mount_point);
  int MakeMountPoint(const std::string &mount_point);

 public:
  FileStore();
  ~FileStore();

  static FileStore *Instance();

  const uint64_t GetMaxSupportFileNumber() noexcept override;
  const uint64_t GetMaxSupportDirectoryNumber() noexcept override;
  const uint64_t GetMaxSupportBlockNumer() noexcept override;
  const uint64_t GetMaxBlockNumber() noexcept override;
  const uint64_t GetFreeBlockNumber() noexcept override;
  const uint64_t GetBlockSize() noexcept override;
  const uint64_t GetFreeFileNumber() noexcept override;
  const uint64_t GetMaxFileMetaSize() noexcept override;
  const uint64_t GetMaxFileNameLength() noexcept override;

  // Mount FileSystem
  int32_t MountFileSystem(const std::string &config_path) override;
  int32_t MountGrowfs(uint64_t size) override;
  int32_t RemountFileSystem() override;
  int32_t UnmountFileSystem() override;

  // Create directory
  int32_t CreateDirectory(const std::string &path) override;
  int32_t NewDirectory(const std::string &dirname,
                       std::unique_ptr<Directory> *result) override;
  // List Directory
  int32_t ListDirectory(const std::string &path, FileInfo **filelist,
                        int *num) override;
  // Delete Directory
  int32_t DeleteDirectory(const std::string &path,
                          bool recursive = false) override;
  // Lock Directory
  int32_t LockDirectory(const std::string &path) override;
  // Unlock Directory
  int32_t UnlockDirectory(const std::string &path) override;

  BLOCKFS_DIR *OpenDirectory(const std::string &dirname);
  block_fs_dirent *ReadDirectory(BLOCKFS_DIR *dir);
  int32_t ReadDirectoryR(BLOCKFS_DIR *dir, block_fs_dirent *entry,
                         block_fs_dirent **result);
  int32_t CloseDirectory(BLOCKFS_DIR *dir) override;
  int32_t ChangeWorkDirectory(const std::string &path);

  int32_t GetWorkDirectory(std::string &path) override;

  // Du
  int32_t DiskUsage(const std::string &path, int64_t *du_size) override;
  // Access
  int32_t Access(const std::string &path, int32_t mode) override;
  // Stat
  int32_t StatPath(const std::string &path, struct stat *fileinfo) override;
  int32_t StatPath(const int32_t fd, struct stat *fileinfo) override;
  int32_t StatVFS(const std::string &path,
                  struct statvfs *buf) override;
  int32_t StatVFS(const int32_t fd, struct statvfs *buf) override;
  // GetFileSize: get real file size
  int32_t GetFileSize(const std::string &path, int64_t *file_size) override;

  // Open file for read or write, flags: O_WRONLY or O_RDONLY
  int32_t OpenFile(const std::string &path, int32_t flags,
                   int32_t mode) override;
  int32_t CloseFile(int32_t fd) override;
  int32_t FileExists(const std::string &path) override;
  int32_t CreateFile(const std::string &path, mode_t mode) override;
  int32_t DeleteFile(const std::string &path) override;
  int32_t RenamePath(const std::string &src,
                     const std::string &target) override;
  // Returns 0 on success.
  int32_t CopyFile(const std::string &from, const std::string &to) override;
  int32_t TruncateFile(const std::string &filename, int64_t size) override;
  int32_t TruncateFile(const int32_t fd, int64_t size) override;
  int32_t PosixFallocate(int32_t fd, int64_t offset, int64_t len) override;

  int32_t ChmodPath(const std::string &path, int32_t mode) override;
  int32_t GetFileModificationTime(const std::string &filename,
                                  uint64_t *filemtime) override;

  int64_t ReadFile(int32_t fd, void *buf, size_t len) override;
  int64_t WriteFile(int32_t fd, const void *buf, size_t len) override;
  int64_t PreadFile(int32_t fd, void *buf, size_t len, off_t offset) override;
  int64_t PwriteFile(int32_t fd, const void *buf, size_t len,
                     off_t offset) override;
  off_t SeekFile(int32_t fd, off_t offset, int whence) override;
  int32_t FcntlFile(int32_t fd, int32_t set_flag) override;
  int32_t FcntlFile(int32_t fd, int16_t lock_type) override;
  int32_t Sync() override;
  int32_t FileSync(const int32_t fd) override;
  int32_t FileDataSync(const int32_t fd) override;
  int32_t FileDup(const int32_t fd) override;
  int32_t RemovePath(const std::string &path) override;
  // Show system status
  int32_t SysStat(const std::string &stat_name, std::string *result) override;

  int32_t LockFile(const std::string &fname) override;
  int32_t UnlockFile(const std::string &fname) override;

  int32_t GetAbsolutePath(const std::string &in_path,
                          std::string *output_path) override;

  void DumpFileMeta(const std::string &path) override;

  BlockDevice *dev() { return device_; }
  ShmManager *shm_manager() { return shm_manager_; }

  MetaHandle *GetMetaHandle(MetaHandleType type) {
    return handle_vector_[type];
  }

  SuperBlock *super() {
    return dynamic_cast<SuperBlock *>(GetMetaHandle(kSuperBlockHandle));
  }
  SuperBlockMeta *super_meta() { return super()->meta(); }

  FdHandle *fd_handle() {
    return dynamic_cast<FdHandle *>(GetMetaHandle(kFDHandle));
  }
  BlockHandle *block_handle() {
    return dynamic_cast<BlockHandle *>(GetMetaHandle(kBlockHandle));
  }
  DirHandle *dir_handle() {
    return dynamic_cast<DirHandle *>(GetMetaHandle(kDirectoryHandle));
  }
  FileHandle *file_handle() {
    return dynamic_cast<FileHandle *>(GetMetaHandle(kFileHandle));
  }
  FileBlockHandle *file_block_handle() {
    return dynamic_cast<FileBlockHandle *>(GetMetaHandle(kFileBlockHandle));
  }

  bool Format(const std::string &dev_name);
  bool Check(const std::string &dev_name, const std::string &log_level = "DEBUG");

  bool CheckPermission(const char *op, const char *path);
};
}  // namespace blockfs
}  // namespace udisk
#endif