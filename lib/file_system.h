#pragma once

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)

#include <fuse3/fuse_lowlevel.h>
#include <stdint.h>
#include <sys/statvfs.h>

#include <memory>
#include <string>

#include "aligned_buffer.h"
#include "comm_utils.h"
#include "bfs_fuse.h"
#include "block_handle.h"
#include "dir_handle.h"
#include "fd_handle.h"
#include "file_block_handle.h"
#include "shm_manager.h"
#include "super_block.h"

namespace udisk::blockfs {

enum MetaHandleType {
  kSuperBlockHandle,
  kBlockHandle,
  kDirectoryHandle,
  kFileHandle,
  kFileBlockHandle,
  kMetaHandleSize,
};

class FileSystem {
 private:
  bfs_config_info mount_config_;

  Device *device_;
  ShmManager *shm_manager_;
  FdHandle *fd_handle_;

  MetaHandle *handle_vector_[kMetaHandleSize];

  static FileSystem *g_instance;

 private:
  bool FormatFSData();
  bool Initialize();
  void Destroy();

  bool OpenTarget(const std::string &uuid);
  bool InitializeMeta();
  int MakeMountPoint(const std::string &mount_point);

 public:
  FileSystem() = default;
  ~FileSystem();

  static FileSystem *Instance();

  bfs_config_info* mount_config() { return &mount_config_; }

  // Mount FileSystem
  int32_t MountFileSystem(const std::string &config_path);

  block_fs_dirent *ReadDirectory(BLOCKFS_DIR *dir);

  // Stat
  int32_t StatPath(const std::string &path, struct stat *fileinfo);
  int32_t StatPath(const int32_t fd, struct stat *fileinfo);
  int32_t StatVFS(struct statvfs *buf);

  int32_t CreateFile(const std::string &path, mode_t mode);
  int32_t RenamePath(const std::string &src,
                     const std::string &target);
  // Returns 0 on success.
  int32_t TruncateFile(const std::string &filename, int64_t size);
  int32_t TruncateFile(const int32_t fd, int64_t size);

  int64_t ReadFile(int32_t fd, void *buf, size_t len);
  int64_t PreadFile(ino_t fd, void *buf, size_t len, off_t offset);
  int64_t PwriteFile(ino_t fd, const void *buf, size_t len,
                     off_t offset);
  off_t SeekFile(ino_t fd, off_t offset, int whence);
  int32_t FcntlFile(int32_t fd, int16_t lock_type);

  void DumpFileMeta(const std::string &path);

  Device *dev() { return device_; }

  MetaHandle *GetMetaHandle(MetaHandleType type) {
    return handle_vector_[type];
  }

  SuperBlock *super() {
    return dynamic_cast<SuperBlock *>(GetMetaHandle(kSuperBlockHandle));
  }
  SuperBlockMeta *super_meta() { return super()->meta(); }

  FdHandle *fd_handle() {
    return fd_handle_;
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