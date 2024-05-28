#ifndef INC_BLOCK_FS_H_
#define INC_BLOCK_FS_H_

#include <dirent.h>
#ifdef __cplusplus
#include <stdint.h>
#else
#include <stdbool.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

#include "block_fs_internal.h"

/**
 * Remove a given dir
 *
 * \param dirname Dir absolute path
 *
 * \return dir remove retcode
 */
int block_fs_rmdir(const char *dirname);

typedef struct blockfs_dirent {
  long d_ino;                /* inode number */
  off_t d_off;               /* offset to this dirent */
  uint32_t d_reclen;         /* length of this d_name */
  uint32_t d_type;           /* the type of d_name */
  uint32_t d_link;           /* link conut */
  char d_name[NAME_MAX + 1]; /* file name (null-terminated) */
  time_t d_time_;
} block_fs_dirent;

/**
 * Read a directory entry from DIRP.  Return a pointer to a `struct
 * dirent' describing the entry, or NULL for EOF or error.  The
 * storage returned may be overwritten by a later readdir call on the
 * same DIR stream.

 * If the Large File Support API is selected we have to use the
 * appropriate interface.
 *
 * This function is a possible cancellation
 * point and therefore not marked with __THROW.
 *
 * \param dir BLOCKFS_DIR opened
 *
 * \return struct dirent* head
 */
struct blockfs_dirent *block_fs_readdir(BLOCKFS_DIR *dir);

/**
 * Dup a given file fd, copy old fd to new fd,
 * new fd reference to the same file,
 * Share all locks, read and write pointers,
 * and various permissions or flags
 *
 * \param oldfd Old file fd(fd)
 *
 * \return File new fd
 */
int block_fs_dup(int oldfd);

/**
 * Truncate a given file path from offset 0
 *
 * \param valpath File absolute path
 * \param len Length want to truncate
 *
 * \return File truncate retcode
 */
int block_fs_truncate(const char *valpath, int64_t len);

/**
 * Truncate a given Fd from offset 0
 *
 * \param fd File fd
 * \param len Length want to truncate
 *
 * \return File truncate retcode
 */
int block_fs_ftruncate(int fd, int64_t len);

/**
 * Truncate a given Fd
 *
 * \param fd File fd
 * \param offset Sepecific offset want to truncate
 * \param len Length want to truncate
 *
 * \return File truncate retcode
 */
int block_fs_posix_fallocate(int fd, uint64_t offset, uint64_t len);

/**
 * Sync all the meta and data
 * used to sync file timestamp
 *
 * \param void
 *
 * \return File sync retcode
 */
int block_fs_sync();

/**
 * Sync a given fd
 *
 * \param fd File fd
 *
 * \return File sync retcode
 */
int block_fs_fsync(int fd);

/**
 * Sync data with a given fd
 *
 * \param fd File fd
 *
 * \return File sync retcode
 */
int block_fs_fdatasync(int fd);

/**
 * File read from current pos
 *
 * \param fd File fd
 * \param buf Read buffer
 * \param len Read length
 *
 * \return read bytes
 */
ssize_t block_fs_read(int fd, void *buf, size_t len);

/**
 * Get dir or file stat
 *
 * \param path Dir or file path
 * \param buf Buffer of struct stat
 *
 * \return chmod retcode
 */
int block_fs_stat(const char *valpath, struct stat *buf);

/**
 * Get dir or file stat
 *
 * \param fd Dir or file fd
 * \param buf Buffer of struct stat
 *
 * \return chmod retcode
 */
int block_fs_fstat(int fd, struct stat *buf);

/**
 * Rename dir or file
 *
 * \param oldpath Old dir or file path
 * \param newpath New dir or file path
 *
 * \return rename retcode
 */
int block_fs_rename(const char *oldpath, const char *newpath);

typedef struct {
  int32_t fd_;
  int64_t offset_;
} BLOCKFS_FILE;

int block_fs_fcntl(int fd, int cmd, ...);

int block_fs_statvfs(const char *path, struct statvfs *buf);
int block_fs_fstatvfs(int fd, struct statvfs *buf);

void block_fs_set_errno(int e);

#endif