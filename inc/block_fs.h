#ifndef INC_BLOCK_FS_H_
#define INC_BLOCK_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#ifdef __cplusplus
#include <stdint.h>
#else
#include <stdbool.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

/**
 * BlockFS mount use to init block file system, load all meta
 *
 * \param config_path Config path, such as /xxxx/bfs.conf
 * \param is_master Running role of this blockfs: master or slave.
 *
 * \return mount retcode
 */
int block_fs_mount(const char *config_path, bool is_master);

/**
 * BlockFS unmount use to uninit block file system, unload all meta
 * Notice: Only one block device mounted by a process supported
 *
 * \param uuid The device uuid generated when format blockfs
 *
 * \return umount retcode
 */
int block_fs_unmount(const char *uuid);

/**
 * Make a given dir
 * Support for directories is very limited at this time
 * mkdir simply puts an entry into the filelist for the
 * requested directory (assuming it does not exist)
 * It doesn't check to see if parent directory exists
 *
 * \param valpath Dir absolute path
 * \param mode Dir mode
 *
 * \return dir create retcode
 */
int block_fs_mkdir(const char *valpath, mode_t mode);

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

typedef struct BLOCKFS_DIR_S BLOCKFS_DIR;

/**
 * Open a directory stream on NAME.
 * Return a DIR stream on the directory,
 * or NULL if it could not be opened.
 *
 * This function is a possible cancellation
 * point and therefore not marked with __THROW.
 *
 * \param name dir path name
 *
 * \return BLOCKFS_DIR*
 */
BLOCKFS_DIR *block_fs_opendir(const char *name);

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
 * Reentrant version of `readdir'.
 * Return in RESULT a pointer to the next entry.
 *
 * This function is a possible cancellation
 * point and therefore not marked with __THROW.
 *
 * \param dir BLOCKFS_DIR opened
 *
 * \return struct blockfs_dirent* head
 */
int block_fs_readdir_r(BLOCKFS_DIR *dir, struct blockfs_dirent *entry,
                       struct blockfs_dirent **result);

/**
 * Close the directory stream DIRP.
 * Return 0 if successful, -1 if not.
 *
 * This function is a possible cancellation
 * point and therefore not marked with __THROW.
 *
 * \param dir BLOCKFS_DIR opened
 *
 * \return 0 success, -1 failed
 */
int block_fs_closedir(BLOCKFS_DIR *dir);

/**
 * Make a given file
 * Create and open FILE, with mode MODE.  This takes an `int' MODE
 * argument because that is what `mode_t' will be widened to
 *
 * \param valpath File absolute path
 * \param mode Dir mode
 *
 * \return File create retcode
 */
int block_fs_create(const char *filename, mode_t mode);

/**
 * Close a given file fd
 *
 * \param fd File fd(fd)
 *
 * \return File close retcode
 */
int block_fs_close(int fd);

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
 * Adjust offset
 *
 * \param fd File fd
 * \param offset File offset
 * \param whence Adjust type
 *
 * \return File lseek retcode
 */
off_t block_fs_lseek(int fd, off_t offset, int whence);

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
 * BLOCKFS_FILE write from current pos
 *
 * \param fd BLOCKFS_FILE fd
 * \param buf Write buffer
 * \param len Write length
 *
 * \return write bytes
 */
ssize_t block_fs_write(int fd, void *buf, size_t len);

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
 * Unlink dir or file, only dec refcnt
 * when equal zero, remove completely
 *
 * \param valpath Dir or file path
 *
 * \return unlink retcode
 */
int block_fs_unlink(const char *filename);

/**
 * Remove dir or file forcely
 *
 * \param valpath Dir or file path
 *
 * \return remove retcode
 */
int block_fs_remove(const char *filename);

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

#ifdef __cplusplus
}
#endif
#endif