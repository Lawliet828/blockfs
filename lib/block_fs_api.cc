#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "block_fs_internal.h"
#include "file_store_udisk.h"
#include "logging.h"

using namespace udisk::blockfs;

int block_fs_mount(const char *config_path, bool is_master) {
  if (FileStore::Instance()->MountFileSystem(config_path) < 0) {
    return -1;
  }
  if (FileStore::Instance()->MountFileLock() < 0) {
    return -1;
  }
  block_fs_fuse_mount(FileStore::Instance()->mount_config());
  return 0;
}

int block_fs_unmount(const char *uuid) {
  return FileStore::Instance()->UnmountFileSystem();
}

int block_fs_resizefs(uint64_t size) {
  return FileStore::Instance()->MountGrowfs(size);
}

const char *block_fs_get_version() {
  return FileStore::Instance()->GetVersion();
}

bool block_fs_is_mounted() { return FileStore::Instance()->is_mounted(); }

bool block_fs_is_master() { return FileStore::Instance()->is_master(); }

int block_fs_remove(const char *path) {
  return FileStore::Instance()->RemovePath(path);
}

int block_fs_rename(const char *oldpath, const char *newpath) {
  return FileStore::Instance()->RenamePath(oldpath, newpath);
}

int block_fs_chmod(const char *path, mode_t mode) {
  return FileStore::Instance()->ChmodPath(path, mode);
}

int block_fs_fchmod(int fd, mode_t mode) { return 0; }

int block_fs_access(const char *path, int amode) {
  return FileStore::Instance()->Access(path, amode);
}

int block_fs_stat(const char *valpath, struct stat *buf) {
  return FileStore::Instance()->StatPath(valpath, buf);
}

int block_fs_fstat(int fd, struct stat *buf) {
  return FileStore::Instance()->StatPath(fd, buf);
}

int block_fs_statvfs(const char *path, struct block_fs_statvfs *buf) {
  return FileStore::Instance()->StatVFS(path, buf);
}

int block_fs_fstatvfs(int fd, struct block_fs_statvfs *buf) {
  return FileStore::Instance()->StatVFS(fd, buf);
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
      return FileStore::Instance()->FcntlFile(fd, flag);
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
      return FileStore::Instance()->FcntlFile(fd, fl->l_type);
    } break;
    default:
      return -1;
  }
}

int block_fs_mkdir(const char *dirname, mode_t mode) {
  return FileStore::Instance()->CreateDirectory(dirname);
}

BLOCKFS_DIR *block_fs_opendir(const char *name) {
  return FileStore::Instance()->OpenDirectory(name);
}

struct blockfs_dirent *block_fs_readdir(BLOCKFS_DIR *dir) {
  return FileStore::Instance()->ReadDirectory(dir);
}

int block_fs_readdir_r(BLOCKFS_DIR *dir, struct blockfs_dirent *entry,
                       struct blockfs_dirent **result) {
  return FileStore::Instance()->ReadDirectoryR(dir, entry, result);
}

int block_fs_closedir(BLOCKFS_DIR *dir) {
  return FileStore::Instance()->CloseDirectory(dir);
}

int block_fs_rmdir(const char *dirname) {
  return FileStore::Instance()->DeleteDirectory(dirname);
}

int block_fs_chdir(const char *dirname) {
  return FileStore::Instance()->ChangeWorkDirectory(dirname);
}

int block_fs_getwd(char *buf, int32_t size) {
  std::string work_dir;
  FileStore::Instance()->GetWorkDirectory(work_dir);
  ::memcpy(buf, work_dir.c_str(), size);
  return 0;
}

int block_fs_create(const char *filename, mode_t mode) {
  return FileStore::Instance()->CreateFile(filename, mode);
}

int block_fs_symlink(const char *from, const char *to) {
  return FileStore::Instance()->Symlink(from, to);
}

int block_fs_readlink(const char *path, char *buf, size_t size) {
  return FileStore::Instance()->ReadLink(path, buf, size);
}

int block_fs_unlink(const char *valpath) {
  return FileStore::Instance()->DeleteFile(valpath);
}

int block_fs_truncate(const char *valpath, int64_t len) {
  if (unlikely(nullptr == valpath)) {
    block_fs_set_errno(EFAULT);
    return -1;
  }
  return FileStore::Instance()->TruncateFile(valpath, len);
}

int block_fs_ftruncate(int fd, int64_t len) {
  return FileStore::Instance()->TruncateFile(fd, len);
}

int block_fs_posix_fallocate(int fd, uint64_t offset, uint64_t len) {
  return FileStore::Instance()->PosixFallocate(fd, offset, len);
}

int block_fs_open(const char *valpath, int flags, ...) {
  /* if O_CREAT is set, we should also have some mode flags */
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arg;
    ::va_start(arg, flags);
    mode = va_arg(arg, int32_t);
    ::va_end(arg);
  }
  return FileStore::Instance()->OpenFile(valpath, flags, mode);
}

int block_fs_close(int fd) { return FileStore::Instance()->CloseFile(fd); }

int block_fs_dup(int oldfd) { return FileStore::Instance()->FileDup(oldfd); }

int block_fs_sync() { return FileStore::Instance()->Sync(); }

int block_fs_fsync(int fd) { return FileStore::Instance()->FileSync(fd); }

int block_fs_fdatasync(int fd) {
  return FileStore::Instance()->FileDataSync(fd);
}

off_t block_fs_lseek(int fd, off_t offset, int whence) {
  return FileStore::Instance()->SeekFile(fd, offset, whence);
}

ssize_t block_fs_read(int fd, void *buf, size_t len) {
  return FileStore::Instance()->ReadFile(fd, buf, len);
}

ssize_t block_fs_write(int fd, void *buf, size_t len) {
  return FileStore::Instance()->WriteFile(fd, buf, len);
}

ssize_t block_fs_pread(int fd, void *buf, size_t len, off_t offset) {
  return FileStore::Instance()->PreadFile(fd, buf, len, offset);
}

ssize_t block_fs_pwrite(int fd, void *buf, size_t len, off_t offset) {
  return FileStore::Instance()->PwriteFile(fd, buf, len, offset);
}

BLOCKFS_FILE *block_fs_fopen(const char *filename, const char *mode) {
  return FileStore::Instance()->FileOpen(filename, mode);
}
int block_fs_fclose(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileClose(stream);
}
int block_fs_fflush(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileFlush(stream);
}
int block_fs_fputc(int c, BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FilePutc(c, stream);
}
int block_fs_fgetc(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileGetc(stream);
}
int block_fs_fputs(const char *str, BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FilePuts(str, stream);
}
char *block_fs_fgets(char *str, int n, BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileGets(str, n, stream);
}

int block_fs_fseek(BLOCKFS_FILE *stream, off_t offset, int whence) {
  return FileStore::Instance()->FileSeek(stream, offset, whence);
}

int block_fs_fprintf(BLOCKFS_FILE *stream, const char *format, ...) {
  // return FileStore::Instance()->FilePrintf(stream, format, ##__VA_ARGS__);
  return -1;
}

int block_fs_fscanf(BLOCKFS_FILE *stream, const char *format, ...) {
  // return FileStore::Instance()->FileScanf(stream, format, ...);
  return -1;
}

int block_fs_feof(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileEof(stream);
}

int block_fs_ferror(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileError(stream);
}

int block_fs_rewind(BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileRewind(stream);
}

size_t block_fs_fread(void *ptr, size_t size, size_t nmemb,
                      BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileRead(ptr, size, nmemb, stream);
}

size_t block_fs_fwrite(const void *ptr, size_t size, size_t nmemb,
                       BLOCKFS_FILE *stream) {
  return FileStore::Instance()->FileWrite(ptr, size, nmemb, stream);
}

int block_fs_mkstemp(char *template_str) {
  return FileStore::Instance()->MksTemp(template_str);
}

BLOCKFS_FILE *block_fs_tmpfile(void) {
  return FileStore::Instance()->TmpFile();
}

char *block_fs_mktemp(char *template_str) {
  return FileStore::Instance()->MkTemp(template_str);
}

char *block_fs_tmpnam(char *str) { return FileStore::Instance()->TmpNam(str); }

ssize_t block_fs_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
  return FileStore::Instance()->SendFile(out_fd, in_fd, offset, count);
}

void block_fs_set_errno(int e) { errno = e; }

int block_fs_get_errno() { return errno; }

bool block_fs_set_available_size(uint64_t available_size) {
  return FileStore::Instance()->super()->set_available_udisk_size(available_size);
}

uint64_t block_fs_get_available_size() {
  return FileStore::Instance()->super()->get_available_udisk_size();
}

bool block_fs_log_utc() {
  return FileStore::Instance()->mount_config()->log_time_utc_;
}
