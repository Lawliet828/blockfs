// Copyright (c) 2020 UCloud All rights reserved.
#include "bfs_fuse.h"

#include <fuse3/fuse.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h> /* flock(2) */
#include <sys/stat.h>

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "block_fs.h"
#include "comm_utils.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

// #define BFS_USE_READ_WRITE_BUFFER

#define DIR_FILLER(F, B, N, S, O) F(B, N, S, O, FUSE_FILL_DIR_PLUS)

#ifndef __NR_copy_file_range
#define __NR_copy_file_range (-1)
loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out,
                       size_t len, unsigned int flags) {
  return syscall(__NR_copy_file_range, fd_in, off_in, fd_out, off_out, len,
                 flags);
}

#endif

class UDiskBFS {
 public:
  UDiskBFS() = default;
  ~UDiskBFS() = default;

  static void FuseLoop(bfs_config_info *info);

  const std::string &uxdb_mount_point() { return uxdb_mount_point_; }

  void RunFuse(bfs_config_info *info) {
    info_ = info;
    // 去掉后面的反斜杠,FUSE挂载默认会带
    uxdb_mount_point_ = info_->uxdb_mount_point_;
    if (uxdb_mount_point_[uxdb_mount_point_.size() - 1] == '/') {
      uxdb_mount_point_.erase(uxdb_mount_point_.size() - 1);
    }
    FuseLoop(info);
    // std::thread fuse_thread(FuseLoop, info);
    // fuse_thread.detach();
  }

  void StopFuse() {
  }

  uint64_t PushOpenDirectory(BLOCKFS_DIR *dp) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t key = (uint64_t)dp;
    open_dirs_[key] = dp;
    return key;
  }

  BLOCKFS_DIR *GetOpenDirectory(uint64_t key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = open_dirs_.find(key);
    if (it == open_dirs_.end()) {
      return nullptr;
    }
    return it->second;
  }

  BLOCKFS_DIR *PopOpenDirectory(uint64_t key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = open_dirs_.find(key);
    if (it == open_dirs_.end()) {
      return nullptr;
    }
    BLOCKFS_DIR *dp = it->second;
    open_dirs_.erase(it);
    return dp;
  }

  static UDiskBFS *Instance() {
    static UDiskBFS *g_instance = nullptr;
    if (!g_instance) {
      static std::once_flag once_flag;
      std::call_once(once_flag, [&]() { g_instance = new UDiskBFS(); });
    }
    return g_instance;
  }

  static void Destroy() {
    Instance()->StopFuse();
    delete Instance();
  }

 private:
  struct fuse_config fuse_cfg_;
  bfs_config_info *info_;

  std::string uxdb_mount_point_;

  std::mutex mutex_;
  std::map<uint64_t, BLOCKFS_DIR *> open_dirs_;
};

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored. The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given. In that case it is passed to userspace,
 * but libfuse and the kernel will still assign a different
 * inode for internal use (called the "nodeid").
 *
 * `fi` will always be nullptr if the file is not currently open, but
 * may also be nullptr if the file is open.
 */
#ifdef HAVE_FUSE3
static int bfs_getattr(const char *path, struct stat *sb,
                       struct fuse_file_info *fi)
#else
static int bfs_getattr(const char *path, struct stat *sb)
#endif
{
  LOG(INFO) << "call bfs_getattr file: " << path;

  int res;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  ::memset(sb, 0, sizeof(struct stat));

  if (::strcmp(path, "/") == 0) {
    sb->st_mode = S_IFDIR | 0755;
    sb->st_nlink = 2;
    sb->st_uid = ::getuid();
    sb->st_gid = ::getgid();

    struct statvfs vfs;
    if (block_fs_statvfs(in_path.c_str(), &vfs) < 0) {
      return -errno;
    }

    time_t now = ::time(NULL);
    sb->st_size = 1024;
    sb->st_blksize = vfs.f_bsize;
    sb->st_blocks = 1;
    sb->st_atime = now;
    sb->st_mtime = now;
    sb->st_ctime = now;

    return 0;
  }

  // fuse_get_context()->gid
  // fuse_get_context()->uid

  if (fi)
    res = block_fs_fstat(fi->fh, sb);
  else
    res = block_fs_stat(in_path.c_str(), sb);
  if (res < 0) return -errno;

  return 0;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
static int bfs_readlink(const char *path, char *buf, size_t size) {
  LOG(INFO) << "call bfs_readlink file: " << path;

  int res;

  res = block_fs_readlink(path, buf, size - 1);
  if (res < 0) return -errno;

  buf[res] = '\0';
  return 0;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
static int bfs_mknod(const char *path, mode_t mode, dev_t rdev) {
  LOG(INFO) << "call bfs_mknod file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  // res = posix_mknod_wrapper(AT_FDCWD, path, nullptr, mode, rdev);
  res = block_fs_create(in_path.c_str(), mode);
  if (res < 0) return -errno;

  return 0;
}

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
static int bfs_mkdir(const char *path, mode_t mode) {
  LOG(INFO) << "call bfs_mkdir file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  res = block_fs_mkdir(in_path.c_str(), mode);
  if (res < 0) return -errno;

  return 0;
}

/** Remove a file */
static int bfs_unlink(const char *path) {
  LOG(INFO) << "call bfs_unlink file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  res = block_fs_unlink(in_path.c_str());
  if (res < 0) return -errno;

  return 0;
}

/** Remove a directory */
static int bfs_rmdir(const char *path) {
  LOG(INFO) << "call bfs_rmdir file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  res = block_fs_rmdir(in_path.c_str());
  if (res < 0) return -errno;

  return 0;
}

/** Create a symbolic link */
int bfs_symlink(const char *from, const char *to) {
  LOG(INFO) << "call bfs_symlink from: " << from << " to: " << to;

  std::string from_path = UDiskBFS::Instance()->uxdb_mount_point();
  std::string to_path = UDiskBFS::Instance()->uxdb_mount_point();
  from_path += from;
  to_path += to;

  int res;

  res = block_fs_symlink(from_path.c_str(), to_path.c_str());
  if (res < 0) return -errno;

  return 0;
}

/** Rename a file
 *
 * *flags* may be `RENAME_EXCHANGE` or `RENAME_NOREPLACE`. If
 * RENAME_NOREPLACE is specified, the filesystem must not
 * overwrite *newname* if it exists and return an error
 * instead. If `RENAME_EXCHANGE` is specified, the filesystem
 * must atomically exchange the two files, i.e. both must
 * exist and neither may be deleted.
 */
static int bfs_rename(const char *from, const char *to, unsigned int flags) {
  LOG(INFO) << "call bfs_rename from: " << from << " to: " << to;

  std::string from_path = UDiskBFS::Instance()->uxdb_mount_point();
  std::string to_path = UDiskBFS::Instance()->uxdb_mount_point();
  from_path += from;
  to_path += to;

  int res;
  /* When we have renameat2() in libc, then we can implement flags */
  if (flags) return -EINVAL;

  res = block_fs_rename(from_path.c_str(), to_path.c_str());
  if (res < 0) return -errno;

  return 0;
}

/** Create a hard link to a file */
int bfs_link(const char *from, const char *to) {
  LOG(INFO) << "call bfs_rename from: " << from << " to: " << to;

  std::string from_path = UDiskBFS::Instance()->uxdb_mount_point();
  std::string to_path = UDiskBFS::Instance()->uxdb_mount_point();
  from_path += from;
  to_path += to;

  // 尚未实现
  return -1;
}

/** Change the permission bits of a file
 *
 * `fi` will always be nullptr if the file is not currenlty open, but
 * may also be nullptr if the file is open.
 */
#ifdef HAVE_FUSE3
static int bfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi)
#else
static int bfs_chmod(const char *path, mode_t mode)
#endif
{
  LOG(INFO) << "call bfs_chmod file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  if (fi)
    res = block_fs_fchmod(fi->fh, mode);
  else
    res = block_fs_chmod(in_path.c_str(), mode);
  if (res < 0) return -errno;

  return 0;
}

/** Change the owner and group of a file
 *
 * `fi` will always be nullptr if the file is not currenlty open, but
 * may also be nullptr if the file is open.
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
#ifdef HAVE_FUSE3
static int bfs_chown(const char *path, uid_t uid, gid_t gid,
                     struct fuse_file_info *fi)
#else
static int bfs_chown(const char *path, uid_t uid, gid_t gid)
#endif
{
  LOG(INFO) << "call bfs_chown file: " << path;

  int res;

  if (fi)
    res = ::fchown(fi->fh, uid, gid);
  else
    res = ::lchown(path, uid, gid);
  if (res < 0) return -errno;

  return 0;
}

/** Change the size of a file
 *
 * `fi` will always be nullptr if the file is not currenlty open, but
 * may also be nullptr if the file is open.
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
#ifdef HAVE_FUSE3
static int bfs_truncate(const char *path, off_t newsize,
                        struct fuse_file_info *fi)
#else
static int bfs_truncate(const char *path, off_t newsize)
#endif
{
  LOG(INFO) << "call bfs_truncate file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int res;

  if (fi)
    res = block_fs_ftruncate(fi->fh, newsize);
  else
    res = block_fs_truncate(in_path.c_str(), newsize);

  if (res < 0) return -errno;

  return 0;
}

/** Open a file
 *
 * Open flags are available in fi->flags. The following rules
 * apply.
 *
 *  - Creation (O_CREAT, O_EXCL, O_NOCTTY) flags will be
 *    filtered out / handled by the kernel.
 *
 *  - Access modes (O_RDONLY, O_WRONLY, O_RDWR, O_EXEC, O_SEARCH)
 *    should be used by the filesystem to check if the operation is
 *    permitted.  If the ``-o default_permissions`` mount option is
 *    given, this check is already done by the kernel before calling
 *    open() and may thus be omitted by the filesystem.
 *
 *  - When writeback caching is enabled, the kernel may send
 *    read requests even for files opened with O_WRONLY. The
 *    filesystem should be prepared to handle this.
 *
 *  - When writeback caching is disabled, the filesystem is
 *    expected to properly handle the O_APPEND flag and ensure
 *    that each write is appending to the end of the file.
 *
 *  - When writeback caching is enabled, the kernel will
 *    handle O_APPEND. However, unless all changes to the file
 *    come through the kernel this will not work reliably. The
 *    filesystem should thus either ignore the O_APPEND flag
 *    (and let the kernel handle it), or return an error
 *    (indicating that reliably O_APPEND is not available).
 *
 * Filesystem may store an arbitrary file handle (pointer,
 * index, etc) in fi->fh, and use this in other all other file
 * operations (read, write, flush, release, fsync).
 *
 * Filesystem may also implement stateless file I/O and not store
 * anything in fi->fh.
 *
 * There are also some flags (direct_io, keep_cache) which the
 * filesystem may set in fi, to change the way the file is opened.
 * See fuse_file_info structure in <fuse_common.h> for more details.
 *
 * If this request is answered with an error code of ENOSYS
 * and FUSE_CAP_NO_OPEN_SUPPORT is set in
 * `fuse_conn_info.capable`, this is treated as success and
 * future calls to open will also succeed without being send
 * to the filesystem process.
 *
 */
static int bfs_open(const char *path, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_open file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int32_t fd = block_fs_open(in_path.c_str(), fi->flags);
  if (fd < 0) {
    return -errno;
  }
  /*
   * fsel files are nonseekable somewhat pipe-like files which
   * gets filled up periodically by producer thread and consumed
   * on read.  Tell FUSE as such.
   */
  fi->fh = fd;
  fi->direct_io = 1;
  // fi->nonseekable = 1;

  /* Make cache persistent even if file is closed,
      this makes it easier to see the effects */
  // fi->keep_cache = 1;
  return 0;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */
static int bfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_read file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (fi == nullptr)
    fd = block_fs_open(in_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  res = block_fs_lseek(fi->fh, offset, SEEK_SET);
  if (res < 0) res = -errno;
  res = block_fs_read(fd, buf, size);
  if (res < 0) res = -errno;

  if (fi == nullptr) block_fs_close(fd);

  return res;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
static int bfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_write file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (fi == nullptr)
    fd = block_fs_open(in_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  res = block_fs_lseek(fd, offset, SEEK_SET);
  if (res < 0) res = -errno;
  res = block_fs_write(fd, const_cast<char *>(buf), size);
  if (res < 0) res = -errno;

  if (fi == nullptr) block_fs_close(fd);

  return res;
}

/** Get file system statistics
 *
 * The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 */
static int bfs_statfs(const char *path, struct statvfs *vfs) {
  LOG(INFO) << "call bfs_statfs file: " << path;

  int res = block_fs_statvfs(path, vfs);
  if (res < 0) return -errno;

  return 0;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor, as opposed to
 * release which is called on the close of the last file descriptor for
 * a file.  Under Linux, errors returned by flush() will be passed to
 * userspace as errors from close(), so flush() is a good place to write
 * back any cached dirty data. However, many applications ignore errors
 * on close(), and on non-Linux systems, close() may succeed even if flush()
 * returns an error. For these reasons, filesystems should not assume
 * that errors returned by flush will ever be noticed or even
 * delivered.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers to an
 * open file handle, e.g. due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush should
 * be treated equally.  Multiple write-flush sequences are relatively
 * rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will be called at any
 * particular point.  It may be called more times than expected, or not
 * at all.
 *
 * [close]: http://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html
 */
static int bfs_flush(const char *path, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_flush file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  /* This is called from every close on an open file, so call the
     close on the underlying filesystem.	But since flush may be
     called multiple times for an open file, this must not really
     close the file.  This is important if used on a network
     filesystem like NFS which flush the data/metadata on close() */
  res = block_fs_close(block_fs_dup(fd));
  if (res < 0) return -errno;

  if (!fi) block_fs_close(fd);

  return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file handle.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
static int bfs_release(const char *path, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_flush file: " << path;

  (void)path;
  int res = 0;

  if (fi) {
    res = block_fs_close(fi->fh);
    if (res < 0) return -errno;
  }
  return -res;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 */
static int bfs_fsync(const char *path, int datasync,
                     struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_fsync file: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

#ifndef HAVE_FDATASYNC
  (void)datasync;
#else
  if (isdatasync)
    res = block_fs_fdatasync(fd);
  else
#endif
  res = block_fs_fsync(fd);

  if (res < 0) return -errno;

  if (!fi) block_fs_close(fd);

  return 0;
}

/** Set extended attributes */
int bfs_setxattr(const char *path, const char *, const char *, size_t, int) {
  LOG(INFO) << "call bfs_setxattr file: " << path;
  return 0;
}

/** Get extended attributes */
int bfs_getxattr(const char *path, const char *, char *, size_t) {
  LOG(INFO) << "call bfs_getxattr file: " << path;
  return 0;
}

/** List extended attributes */
int bfs_listxattr(const char *path, char *, size_t) {
  LOG(INFO) << "call bfs_listxattr file: " << path;
  return 0;
}

/** Remove extended attributes */
int bfs_removexattr(const char *path, const char *) {
  LOG(INFO) << "call bfs_removexattr file: " << path;
  return 0;
}

/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, releasedir and fsyncdir.
 */

static int bfs_opendir(const char *path, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_opendir: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;
  BLOCKFS_DIR *dp = block_fs_opendir(in_path.c_str());
  if (!dp) {
    return -errno;
  }
  if (fi) {
    fi->fh = UDiskBFS::Instance()->PushOpenDirectory(dp);
  } else {
    LOG(FATAL) << "file info cannot empty";
  }
  return 0;
}

/** Read directory
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 */
#ifdef HAVE_FUSE3
static int bfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags)
#else
static int bfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
#endif
{
  LOG(INFO) << "call bfs_readdir: " << path;

  (void)offset;
  (void)fi;
  (void)flags;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  LOG(INFO) << "readdir: " << in_path;

  if (::strcmp(path, "/") == 0) {
    if (DIR_FILLER(filler, buf, ".", nullptr, 0) != 0 ||
        DIR_FILLER(filler, buf, "..", nullptr, 0) != 0) {
      LOG(INFO) << "filler directory: . and ..";
      return 0;
    }
  }
  BLOCKFS_DIR *dp = UDiskBFS::Instance()->GetOpenDirectory(fi->fh);
  if (!dp) {
    LOG(ERROR) << "failed to find open directory: " << path;
    return -EINVAL;
  }
  struct blockfs_dirent *de;
  while ((de = block_fs_readdir(dp)) != nullptr) {
    // struct stat st;
    // memset(&st, 0, sizeof(st));
    // st.st_ino = de->d_ino;
    // st.st_mode = de->d_type << 12;
    // if (filler(buf, de->d_name, &st, 0, (fuse_fill_dir_flags)0))
    //   break;
#if 0
    struct stat st;
    enum fuse_fill_dir_flags fill_flags = (fuse_fill_dir_flags)0;
    if (flags & FUSE_READDIR_PLUS) {
      int res;

      res = block_fs_stat(de->d_name, &st);
      if (res != -1) {
        fill_flags == FUSE_FILL_DIR_PLUS;
      }
    }
    if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
      ::memset(&st, 0, sizeof(st));
      st.st_ino = de->d_ino;
      st.st_mode = de->d_type << 12;
    }

    if (DIR_FILLER(filler, buf, de->d_name, &st, de->d_off) != 0) {
      return -ENOMEM;
    }
#else
    if (DIR_FILLER(filler, buf, de->d_name, 0, 0) != 0) {
      LOG(INFO) << "filler directory, " << de->d_name;
      return -ENOMEM;
    }
#endif
  }
  return 0;
}

/** Release directory
 */
static int bfs_releasedir(const char *path, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_releasedir: " << path;

  if (fi) {
    BLOCKFS_DIR *dp = UDiskBFS::Instance()->PopOpenDirectory(fi->fh);
    if (!dp) {
      LOG(ERROR) << "failed to find open directory: " << path;
      return -EINVAL;
    }
    return -block_fs_closedir(dp);
  }

  return 0;
}

static void bfs_capabilities(struct fuse_conn_info *conn) {
  LOG(INFO) << "Protocol version: " << conn->proto_major << "."
            << conn->proto_minor;
  LOG(INFO) << "Capabilities: ";
  if (conn->capable & FUSE_CAP_WRITEBACK_CACHE)
    LOG(INFO) << "FUSE_CAP_WRITEBACK_CACHE";
  if (conn->capable & FUSE_CAP_ASYNC_READ) LOG(INFO) << "FUSE_CAP_ASYNC_READ";
  if (conn->capable & FUSE_CAP_POSIX_LOCKS) LOG(INFO) << "FUSE_CAP_POSIX_LOCKS";
  if (conn->capable & FUSE_CAP_ATOMIC_O_TRUNC)
    LOG(INFO) << "FUSE_CAP_ATOMIC_O_TRUNC";
  if (conn->capable & FUSE_CAP_EXPORT_SUPPORT)
    LOG(INFO) << "FUSE_CAP_EXPORT_SUPPORT";
  if (conn->capable & FUSE_CAP_DONT_MASK) LOG(INFO) << "FUSE_CAP_DONT_MASK";
  if (conn->capable & FUSE_CAP_SPLICE_MOVE) LOG(INFO) << "FUSE_CAP_SPLICE_MOVE";
  if (conn->capable & FUSE_CAP_SPLICE_READ) LOG(INFO) << "FUSE_CAP_SPLICE_READ";
  if (conn->capable & FUSE_CAP_SPLICE_WRITE)
    LOG(INFO) << "FUSE_CAP_SPLICE_WRITE";
  if (conn->capable & FUSE_CAP_FLOCK_LOCKS) LOG(INFO) << "FUSE_CAP_FLOCK_LOCKS";
  if (conn->capable & FUSE_CAP_IOCTL_DIR) LOG(INFO) << "FUSE_CAP_IOCTL_DIR";
  if (conn->capable & FUSE_CAP_AUTO_INVAL_DATA)
    LOG(INFO) << "FUSE_CAP_AUTO_INVAL_DATA";
  if (conn->capable & FUSE_CAP_READDIRPLUS) LOG(INFO) << "FUSE_CAP_READDIRPLUS";
  if (conn->capable & FUSE_CAP_READDIRPLUS_AUTO)
    LOG(INFO) << "FUSE_CAP_READDIRPLUS_AUTO";
  if (conn->capable & FUSE_CAP_ASYNC_DIO) LOG(INFO) << "FUSE_CAP_ASYNC_DIO";
  if (conn->capable & FUSE_CAP_WRITEBACK_CACHE)
    LOG(INFO) << "FUSE_CAP_WRITEBACK_CACHE";
  if (conn->capable & FUSE_CAP_NO_OPEN_SUPPORT)
    LOG(INFO) << "FUSE_CAP_NO_OPEN_SUPPORT";
  if (conn->capable & FUSE_CAP_PARALLEL_DIROPS)
    LOG(INFO) << "FUSE_CAP_PARALLEL_DIROPS";
  if (conn->capable & FUSE_CAP_POSIX_ACL) LOG(INFO) << "FUSE_CAP_POSIX_ACL";
  if (conn->capable & FUSE_CAP_CACHE_SYMLINKS)
    LOG(INFO) << "FUSE_CAP_CACHE_SYMLINKS";
  if (conn->capable & FUSE_CAP_NO_OPENDIR_SUPPORT)
    LOG(INFO) << "FUSE_CAP_NO_OPENDIR_SUPPORT";
  if (conn->capable & FUSE_CAP_EXPLICIT_INVAL_DATA)
    LOG(INFO) << "FUSE_CAP_EXPLICIT_INVAL_DATA";
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 */
int bfs_fsyncdir(const char *path, int, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_fsyncdir: " << path;
  return -1;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the `private_data` field of
 * `struct fuse_context` to all file operations, and as a
 * parameter to the destroy() method. It overrides the initial
 * value provided to fuse_main() / fuse_new().
 */
static void *bfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  LOG(INFO) << "call bfs_init";

  bfs_capabilities(conn);
  // cfg->kernel_cache = 1;
  // cfg->use_ino = 1;
  // cfg->nullpath_ok = 1;

  /* Pick up changes from lower filesystem right away. This is
     also necessary for better hardlink support. When the kernel
     calls the unlink() handler, it does not know the inode of
     the to-be-removed entry and can therefore not invalidate
     the cache of the associated inode - resulting in an
     incorrect st_nlink value being reported for any remaining
     hardlinks to this inode. */
  // cfg->entry_timeout = 0;
  // cfg->attr_timeout = 0;
  // cfg->negative_timeout = 0;

  // FUSE的big_writes与direct_io选项分析
  // https://www.cnblogs.com/yunnotes/archive/2013/04/19/3032497.html
  cfg->direct_io = 1;

  struct fuse_context *cxt = fuse_get_context();
  (void)cxt;
  return nullptr;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
static void bfs_destroy(void *private_data) {
  LOG(INFO) << "call bfs_destroy";
  UDiskBFS::Destroy();
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 */
static int bfs_access(const char *path, int mask) {
  LOG(INFO) << "call bfs_access: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;
  int res;

  res = block_fs_access(in_path.c_str(), mask);
  if (res < 0) return -errno;

  return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 */
static int bfs_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_create: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;

  fd = block_fs_open(in_path.c_str(), fi->flags, mode);
  if (fd < 0) return -errno;

  fi->fh = fd;
  return 0;
}

/**
 * Perform POSIX file locking operation
 *
 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
 *
 * For the meaning of fields in 'struct flock' see the man page
 * for fcntl(2).  The l_whence field will always be set to
 * SEEK_SET.
 *
 * For checking lock ownership, the 'fuse_file_info->owner'
 * argument must be used.
 *
 * For F_GETLK operation, the library will first check currently
 * held locks, and if a conflicting lock is found it will return
 * information without calling this method.	 This ensures, that
 * for local locks the l_pid field is correctly filled in.	The
 * results may not be accurate in case of race conditions and in
 * the presence of hard links, but it's unlikely that an
 * application would rely on accurate GETLK results in these
 * cases.  If a conflicting lock is not found, this method will be
 * called, and the filesystem may fill out l_pid by a meaningful
 * value, or it may leave this field zero.
 *
 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
 * of the process performing the locking operation.
 *
 * Note: if this method is not implemented, the kernel will still
 * allow file locking to work locally.  Hence it is only
 * interesting for network filesystems and similar.
 */
int bfs_lock(const char *path, struct fuse_file_info *fi, int cmd,
             struct flock *lock) {
  LOG(INFO) << "call bfs_lock: " << path << " fd: " << fi->fh;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_WRONLY);
  else
    fd = fi->fh;

  struct flock fl;
  // 	res = flock(fi->fh, op);
  res = block_fs_fcntl(fd, F_SETLK, &fl);
  if (res < 0) return -errno;

  return 0;
}

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * `fi` will always be nullptr if the file is not currenlty open, but
 * may also be nullptr if the file is open.
 *
 * See the utimensat(2) man page for details.
 */
static int bfs_utimens(const char *path, const struct timespec ts[2],
                       struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_utimens: " << path;

#ifdef HAVE_UTIMENSAT
  int res;
  /* don't use utime/utimes since they follow symlinks */
  if (fi)
    res = ::futimens(fi->fh, ts);
  else
    res = ::utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
  if (res < 0) return -errno;
#endif

  return 0;
}

/**
 * Map block index within file to block index within device
 *
 * Note: This makes sense only for block device backed filesystems
 * mounted with the 'blkdev' option
 */
static int bfs_bmap(const char *path, size_t blocksize, uint64_t *idx) {
  LOG(INFO) << "call bfs_bmap: " << path;
  return -1;
}

/**
 * Ioctl
 *
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be nullptr, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-nullptr cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * If flags has FUSE_IOCTL_DIR then the fuse_file_info refers to a
 * directory file handle.
 *
 * Note : the unsigned long request submitted by the application
 * is truncated to 32 bits.
 */
#if FUSE_USE_VERSION < 35
static int bfs_ioctl(const char *path, int cmd, void *arg,
                     struct fuse_file_info *, unsigned int flags, void *data)
#else
static int bfs_ioctl(const char *path, unsigned int cmd, void *arg,
                     struct fuse_file_info *, unsigned int flags, void *data)
#endif
{
  LOG(INFO) << "call bfs_ioctl: " << path;
  return -1;
}

/**
 * Poll for IO readiness events
 *
 * Note: If ph is non-nullptr, the client should notify
 * when IO readiness events occur by calling
 * fuse_notify_poll() with the specified ph.
 *
 * Regardless of the number of times poll with a non-nullptr ph
 * is received, single notification is enough to clear all.
 * Notifying more times incurs overhead but doesn't harm
 * correctness.
 *
 * The callee is responsible for destroying ph with
 * fuse_pollhandle_destroy() when no longer in use.
 */
static int bfs_poll(const char *path, struct fuse_file_info *fi,
                    struct fuse_pollhandle *ph, unsigned *reventsp) {
  LOG(INFO) << "call bfs_poll: " << path;
  return -1;
}

#ifdef BFS_USE_READ_WRITE_BUFFER
/** Write contents of buffer to an open file
 *
 * Similar to the write() method, but data is supplied in a
 * generic buffer.  Use fuse_buf_copy() to transfer data to
 * the destination.
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
static int bfs_write_buf(const char *path, struct fuse_bufvec *buf,
                         off_t offset, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_write_buf: " << path << " fd: " << fi->fh
            << " offset: " << offset << " count: " << buf->count;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  uint64_t res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  int64_t write_size = fuse_buf_size(buf);
  int64_t write_offset = offset;
  res = block_fs_ftruncate(fd, write_size + write_offset);
  if (res < 0) {
    return -errno;
  }

  res = block_fs_lseek(fd, write_offset, SEEK_SET);
  if (res < 0) return -errno;

  struct fuse_buf *buffer;
  for (size_t i = 0; i < buf->count; ++i) {
    buffer = &buf->buf[i];
    LOG(INFO) << "buffer fd: " << buffer->fd << " mem: " << buffer->mem
              << " size: " << buffer->size << " pos: " << buffer->pos;
    write_size = block_fs_write(fd, buf->buf[i].mem, buffer->size);
    if (write_size < 0) {
      return -errno;
    }
    res += write_size;
  }
  if (fi == nullptr) block_fs_close(fd);

#if 0
  struct fuse_bufvec dst = FUSE_BUFVEC_INIT(fuse_buf_size(buf));
  dst.buf[0].flags =
      static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD);
  dst.buf[0].fd = fi->fh;
  dst.buf[0].pos = offset;
  dst.buf[0].write = block_fs_write;

  res = fuse_buf_copy(&dst, buf, FUSE_BUF_NO_SPLICE);
  if (res < 0) {
    LOG(ERROR) << "bfs_write_buf res: " << res << " errno: " << errno;
  }
#endif

  return res;
}

/** Store data from an open file in a buffer
 *
 * Similar to the read() method, but data is stored and
 * returned in a generic buffer.
 *
 * No actual copying of data has to take place, the source
 * file descriptor may simply be stored in the buffer for
 * later data transfer.
 *
 * The buffer must be allocated dynamically and stored at the
 * location pointed to by bufp.  If the buffer contains memory
 * regions, they too must be allocated using malloc().  The
 * allocated memory will be freed by the caller.
 */
int bfs_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size,
                 off_t offset, struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_read_buf: " << path << " bufp: " << (char *)bufp;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  ssize_t res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  struct fuse_bufvec *src;
  src = (struct fuse_bufvec *)::malloc(sizeof(struct fuse_bufvec));
  if (src == nullptr) return -ENOMEM;

  *src = FUSE_BUFVEC_INIT(size);

  src->count = 1;
  src->buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD);
  src->buf[0].fd = fi->fh;
  src->buf[0].pos = offset;
  src->buf[0].read = block_fs_read;
  src->buf[0].mem = ::malloc(size);
  if (!src->buf[0].mem) {
    ::free(src);
    return -ENOMEM;
  }

  res = block_fs_lseek(fd, offset, SEEK_SET);
  if (res < 0) return -errno;

  res = block_fs_read(fd, src->buf[0].mem, size);
  if (res < 0) return -errno;

  *bufp = src;

#if 0
  res = fuse_buf_copy(src, *bufp, FUSE_BUF_NO_SPLICE);
  if (res < 0) {
    LOG(ERROR) << "bfs_read_buf res: " << res << " errno: " << errno;
  }
#endif

  if (fi == nullptr) block_fs_close(fd);

  return 0;
}
#endif

/**
 * Perform BSD file locking operation
 *
 * The op argument will be either LOCK_SH, LOCK_EX or LOCK_UN
 *
 * Nonblocking requests will be indicated by ORing LOCK_NB to
 * the above operations
 *
 * For more information see the flock(2) manual page.
 *
 * Additionally fi->owner will be set to a value unique to
 * this open file.  This same value will be supplied to
 * ->release() when the file is released.
 *
 * Note: if this method is not implemented, the kernel will still
 * allow file locking to work locally.  Hence it is only
 * interesting for network filesystems and similar.
 */
static int bfs_flock(const char *path, struct fuse_file_info *fi, int op) {
  LOG(INFO) << "call bfs_flock: " << path;

  std::string in_path = UDiskBFS::Instance()->uxdb_mount_point();
  in_path += path;

  int fd;
  int res;

  if (!fi)
    fd = block_fs_open(in_path.c_str(), O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  struct flock fl;
  // 	res = flock(fi->fh, op);
  res = block_fs_fcntl(fd, F_SETLK, &fl);
  if (res < 0) return -errno;

  if (fi == nullptr) block_fs_close(fd);

  return 0;
}

/**
 * Allocates space for an open file
 *
 * This function ensures that required space is allocated for specified
 * file.  If this function returns success then any subsequent write
 * request to specified range is guaranteed not to fail because of lack
 * of space on the file system media.
 */
static int bfs_fallocate(const char *path, int mode, off_t offset, off_t length,
                         struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_fallocate: " << path;

  int fd;
  int res;

  if (mode) return -EOPNOTSUPP;

  if (fi == nullptr)
    fd = block_fs_open(path, O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  res = -block_fs_posix_fallocate(fd, offset, length);

  if (fi == nullptr) block_fs_close(fd);

  return res;
}

/**
 * Copy a range of data from one file to another
 *
 * Performs an optimized copy between two file descriptors without the
 * additional cost of transferring data through the FUSE kernel module
 * to user space (glibc) and then back into the FUSE filesystem again.
 *
 * In case this method is not implemented, applications are expected to
 * fall back to a regular file copy.   (Some glibc versions did this
 * emulation automatically, but the emulation has been removed from all
 * glibc release branches.)
 */
ssize_t bfs_copy_file_range(const char *path_in, struct fuse_file_info *fi_in,
                            off_t offset_in, const char *path_out,
                            struct fuse_file_info *fi_out, off_t offset_out,
                            size_t size, int flags) {
  LOG(INFO) << "call bfs_copy_file_range: " << path_in
            << " path_out: " << path_out;

  int fd_in, fd_out;
  ssize_t res;

  if (fi_in == nullptr)
    fd_in = block_fs_open(path_in, O_RDONLY);
  else
    fd_in = fi_in->fh;

  if (fd_in == -1) return -errno;

  if (fi_out == nullptr)
    fd_out = block_fs_open(path_out, O_WRONLY);
  else
    fd_out = fi_out->fh;

  if (fd_out == -1) {
    block_fs_close(fd_in);
    return -errno;
  }

  res = ::copy_file_range(fi_in->fh, &offset_in, fi_out->fh, &offset_out, size,
                          flags);
  if (res < 0) return -errno;

  if (fi_out == nullptr) block_fs_close(fd_out);
  if (fi_in == nullptr) block_fs_close(fd_in);
  return res;
}

/**
 * Find next data or hole after the specified offset
 */
off_t bfs_lseek(const char *path, off_t off, int whence,
                struct fuse_file_info *fi) {
  LOG(INFO) << "call bfs_lseek: " << path;

  int fd;
  off_t res;

  if (fi == nullptr)
    fd = block_fs_open(path, O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1) return -errno;

  res = block_fs_lseek(fi->fh, off, whence);
  if (res < 0) return -errno;

  if (fi == nullptr) block_fs_close(fd);

  return res;
}

static const struct fuse_operations kBFSOps = {
    .getattr = bfs_getattr,
    .readlink = bfs_readlink,
    .mknod = bfs_mknod,
    .mkdir = bfs_mkdir,
    .unlink = bfs_unlink,
    .rmdir = bfs_rmdir,
    .symlink = bfs_symlink,
    .rename = bfs_rename,
    .link = bfs_link,
    .chmod = bfs_chmod,
    .chown = bfs_chown,
    .truncate = bfs_truncate,
    .open = bfs_open,
    .read = bfs_read,
    .write = bfs_write,
    .statfs = bfs_statfs,
    .flush = bfs_flush,
    .release = bfs_release,
    .fsync = bfs_fsync,
    .setxattr = bfs_setxattr,
    .getxattr = bfs_getxattr,
    .listxattr = bfs_listxattr,
    .removexattr = bfs_removexattr,
    .opendir = bfs_opendir,
    .readdir = bfs_readdir,
    .releasedir = bfs_releasedir,
    .fsyncdir = bfs_fsyncdir,
    .init = bfs_init,
    .destroy = bfs_destroy,
    .access = bfs_access,
    .create = bfs_create,
    .lock = bfs_lock,
    .utimens = bfs_utimens,
    .bmap = bfs_bmap,
    .ioctl = bfs_ioctl,
    .poll = bfs_poll,
#ifdef BFS_USE_READ_WRITE_BUFFER
    .write_buf = bfs_write_buf,
    .read_buf = bfs_read_buf,
#else
    .write_buf = nullptr,
    .read_buf = nullptr,
#endif
    .flock = bfs_flock,
    .fallocate = bfs_fallocate,
    .copy_file_range = bfs_copy_file_range,
    .lseek = bfs_lseek,
};

void UDiskBFS::FuseLoop(bfs_config_info *info) {
  ::umask(0);
  LOG(INFO) << "FUSE version: " << fuse_pkgversion();

#if 0
  auto fuse_thread_func = [](bfs_config_info *info) {
    pthread_setname_np(pthread_self(), "fuse");
    struct fuse_cmdline_opts opts;
    int ret = 0;

    char *mountpoint = (char *)malloc(info->fuse_mount_point.size() + 1);
    memcpy(mountpoint, info->fuse_mount_point.c_str(), info->fuse_mount_point.size());
    mountpoint[info->fuse_mount_point.size()] = 0;
    std::string fsname = "BFS:test";
    char *fsname_str = (char *)malloc(fsname.size() + 1);
    memcpy(fsname_str, fsname.c_str(), fsname.size());
    fsname_str[fsname.size()] = 0;
    std::vector<char *> argv;
    argv.push_back(fsname_str);
    argv.push_back("-f");
    argv.push_back(mountpoint);
    argv.push_back("-oallow_other");
    argv.push_back("-odefault_permissions");
    argv.push_back("-orw"); // 读写权限
    for (size_t i = 0; i < argv.size(); ++i) {
      LOG(INFO) << "fuse args: " << argv[i];
    }

    int argc = argv.size();
    char **argv_ptr = argv.data();
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv_ptr);
    if (fuse_parse_cmdline(&args, &opts) != 0) {
      LOG(ERROR) << "failed to parse fuse command line";
      return;
    }

    struct fuse_session *se = fuse_session_new(
        &args, &kBFSOps, sizeof(kBFSOps), nullptr);
    if (se == nullptr) {
      LOG(FATAL) << "fuse create session failed";
    }
    if ((ret = fuse_set_signal_handlers(se)) != 0) {
      LOG(FATAL) << "fuse set signal handlers failed, ret:" << ret;
    }
    if ((ret = fuse_session_mount(se, opts.mountpoint)) != 0) {
      LOG(FATAL) << "fuse mount dir failed, ret:" << ret;
    }
    // 这里面会fork
    // fuse_daemonize(1);
    LOG(INFO) << "fuse opts idle_threads " << opts.max_idle_threads
              << " max_threads " << opts.max_threads << " clone_fd "
              << opts.clone_fd;
    struct fuse_loop_config *config = fuse_loop_cfg_create();
    fuse_loop_cfg_set_idle_threads(config, -1);
    fuse_loop_cfg_set_max_threads(config, 4);
    fuse_loop_cfg_set_clone_fd(config, 1);
    fuse_session_loop_mt(se, config);
    fuse_session_unmount(se);
    fuse_loop_cfg_destroy(config);

    LOG(INFO) << "fuse umount";
    exit(0);
  };
  std::thread fuse_thread(fuse_thread_func, info);
  fuse_thread.detach();
#endif

  char *mountpoint = (char *)malloc(info->fuse_mount_point.size() + 1);
  memcpy(mountpoint, info->fuse_mount_point.c_str(), info->fuse_mount_point.size());
  mountpoint[info->fuse_mount_point.size()] = 0;
  std::string fsname = "BFS:test";
  char *fsname_str = (char *)malloc(fsname.size() + 1);
  memcpy(fsname_str, fsname.c_str(), fsname.size());
  fsname_str[fsname.size()] = 0;
  std::vector<char *> argv;
  argv.push_back(fsname_str);
  if (info->fuse_debug_) {
    LOG(INFO) << "fuse enable debug";
    argv.push_back((char *)"-d");
  }
  argv.push_back("-f");
  argv.push_back(mountpoint);
  argv.push_back("-oallow_other");
  argv.push_back("-odefault_permissions");
  argv.push_back("-orw"); // 读写权限
  argv.push_back("-oauto_unmount");
  for (size_t i = 0; i < argv.size(); ++i) {
    LOG(INFO) << "fuse args: " << argv[i];
  }

  int argc = argv.size();
  char **argv_ptr = argv.data();
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv_ptr);

  if (!fuse_main(args.argc, args.argv, &kBFSOps, info)) {
    LOG(WARNING) << "fuse mount loop exit";
  }
}

void block_fs_fuse_mount(bfs_config_info *info) {
  if (!RunAsAdmin()) {
    LOG(ERROR) << "fuse mount requires root privileges";
    return;
  }
  UDiskBFS::Instance()->RunFuse(info);
}

}  // namespace blockfs
}  // namespace udisk