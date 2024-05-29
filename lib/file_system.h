#ifndef LIB_FILE_STORE_H_
#define LIB_FILE_STORE_H_

#include <stdint.h>
#include <sys/statvfs.h>

#include <memory>
#include <string>

#include "aligned_buffer.h"
#include "block_fs_internal.h"
#include "block_fs_config.h"
#include "comm_utils.h"
#include "bfs_fuse.h"
#include "block_handle.h"
#include "dir_handle.h"
#include "fd_handle.h"
#include "file_block_handle.h"
#include "shm_manager.h"
#include "super_block.h"

namespace udisk::blockfs {

class Directory;

enum MetaHandleType {
  kSuperBlockHandle,
  kFDHandle,
  kBlockHandle,
  kDirectoryHandle,
  kFileHandle,
  kFileBlockHandle,
  kMetaHandleSize,
};

class FileSystem {
 private:
  bfs_config_info mount_config_;
  bool remount_ = false;

  BlockDevice *device_;
  ShmManager *shm_manager_;

  MetaHandle *handle_vector_[kMetaHandleSize];

  static FileSystem *g_instance;

 private:
  bool FormatFSMeta();
  bool FormatFSData();
  bool Initialize();
  void Destroy();

  bool OpenTarget(const std::string &uuid);
  bool InitializeMeta();
  int MakeMountPoint(const char *mount_point);
  int MakeMountPoint(const std::string &mount_point);

 public:
  FileSystem();
  ~FileSystem();

  static FileSystem *Instance();

  bfs_config_info* mount_config() { return &mount_config_; }

  const uint64_t GetMaxSupportFileNumber() noexcept;
  const uint64_t GetMaxSupportBlockNumer() noexcept;
  const uint64_t GetFreeBlockNumber() noexcept;
  const uint64_t GetBlockSize() noexcept;
  const uint64_t GetFreeFileNumber() noexcept;
  const uint64_t GetMaxFileMetaSize() noexcept;
  const uint64_t GetMaxFileNameLength() noexcept;

  // Mount FileSystem
  int32_t MountFileSystem(const std::string &config_path);
  int32_t RemountFileSystem();

  block_fs_dirent *ReadDirectory(BLOCKFS_DIR *dir);

  // Stat
  int32_t StatPath(const std::string &path, struct stat *fileinfo);
  int32_t StatPath(const int32_t fd, struct stat *fileinfo);
  int32_t StatVFS(const std::string &path,
                  struct statvfs *buf);
  int32_t StatVFS(const ino_t fd, struct statvfs *buf);

  int32_t CreateFile(const std::string &path, mode_t mode);
  int32_t RenamePath(const std::string &src,
                     const std::string &target);
  // Returns 0 on success.
  int32_t TruncateFile(const std::string &filename, int64_t size);
  int32_t TruncateFile(const int32_t fd, int64_t size);
  int32_t PosixFallocate(int32_t fd, int64_t offset, int64_t len);

  int64_t ReadFile(int32_t fd, void *buf, size_t len);
  int64_t WriteFile(int32_t fd, const void *buf, size_t len);
  int64_t PreadFile(ino_t fd, void *buf, size_t len, off_t offset);
  int64_t PwriteFile(ino_t fd, const void *buf, size_t len,
                     off_t offset);
  off_t SeekFile(ino_t fd, off_t offset, int whence);
  int32_t FcntlFile(int32_t fd, int32_t set_flag);
  int32_t FcntlFile(int32_t fd, int16_t lock_type);

  void DumpFileMeta(const std::string &path);

  BlockDevice *dev() { return device_; }

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
};


inline ssize_t block_fs_read(int fd, void *buf, size_t len) {
  return FileSystem::Instance()->ReadFile(fd, buf, len);
}

}
#endif