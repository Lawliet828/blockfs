#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "block_fs_internal.h"
#include "file_system.h"

using namespace udisk::blockfs;

int block_fs_mount(const char *config_path, bool is_master) {
  if (FileSystem::Instance()->MountFileSystem(config_path) < 0) {
    return -1;
  }

  bfs_config_info *conf = FileSystem::Instance()->mount_config();
  conf->fuse_mount_point = "/data/mysql/bfs";

  block_fs_fuse_mount(FileSystem::Instance()->mount_config());
  return 0;
}

int block_fs_unmount(const char *uuid) {
  return FileSystem::Instance()->UnmountFileSystem();
}

int block_fs_rename(const char *oldpath, const char *newpath) {
  return FileSystem::Instance()->RenamePath(oldpath, newpath);
}

int block_fs_stat(const char *valpath, struct stat *buf) {
  return FileSystem::Instance()->StatPath(valpath, buf);
}

int block_fs_fstat(int fd, struct stat *buf) {
  return FileSystem::Instance()->StatPath(fd, buf);
}

int block_fs_statvfs(const char *path, struct statvfs *buf) {
  return FileSystem::Instance()->StatVFS(path, buf);
}

int block_fs_fstatvfs(int fd, struct statvfs *buf) {
  return FileSystem::Instance()->StatVFS(fd, buf);
}

int block_fs_fcntl(int fd, int cmd, ...) {
  switch (cmd) {
    case F_SETFL: {
      /* if F_SETFL is set, we should also have some flags */
      int flag = 0;
      va_list arg;
      ::va_start(arg, cmd);
      flag = va_arg(arg, int32_t);
      ::va_end(arg);
      return FileSystem::Instance()->FcntlFile(fd, flag);
    } break;
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW: {
      /* if LK is set, we should also have some flags */
      struct flock *fl;
      va_list arg;
      va_start(arg, cmd);
      fl = va_arg(arg, struct flock *);
      va_end(arg);
      return FileSystem::Instance()->FcntlFile(fd, fl->l_type);
    } break;
    default:
      return -1;
  }
}

BLOCKFS_DIR *block_fs_opendir(const char *name) {
  return FileSystem::Instance()->OpenDirectory(name);
}

struct blockfs_dirent *block_fs_readdir(BLOCKFS_DIR *dir) {
  return FileSystem::Instance()->ReadDirectory(dir);
}

int block_fs_readdir_r(BLOCKFS_DIR *dir, struct blockfs_dirent *entry,
                       struct blockfs_dirent **result) {
  return FileSystem::Instance()->ReadDirectoryR(dir, entry, result);
}

int block_fs_closedir(BLOCKFS_DIR *dir) {
  return FileSystem::Instance()->CloseDirectory(dir);
}

int block_fs_rmdir(const char *dirname) {
  return FileSystem::Instance()->DeleteDirectory(dirname);
}

int block_fs_truncate(const char *valpath, int64_t len) {
  if (nullptr == valpath) [[unlikely]] {
    block_fs_set_errno(EFAULT);
    return -1;
  }
  return FileSystem::Instance()->TruncateFile(valpath, len);
}

int block_fs_ftruncate(int fd, int64_t len) {
  return FileSystem::Instance()->TruncateFile(fd, len);
}

int block_fs_posix_fallocate(int fd, uint64_t offset, uint64_t len) {
  return FileSystem::Instance()->PosixFallocate(fd, offset, len);
}

int block_fs_dup(int oldfd) { return FileSystem::Instance()->FileDup(oldfd); }

int block_fs_sync() { return FileSystem::Instance()->Sync(); }

int block_fs_fsync(int fd) { return FileSystem::Instance()->FileSync(fd); }

int block_fs_fdatasync(int fd) {
  return FileSystem::Instance()->FileDataSync(fd);
}

ssize_t block_fs_read(int fd, void *buf, size_t len) {
  return FileSystem::Instance()->ReadFile(fd, buf, len);
}

void block_fs_set_errno(int e) { errno = e; }
