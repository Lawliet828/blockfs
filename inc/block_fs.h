// Copyright (c) 2020 UCloud All rights reserved.
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

typedef struct block_fs_mount_stat_t {
  bool is_mounted_;
#ifdef __cplusplus
  block_fs_mount_stat_t() : is_mounted_(false) {}
  void set_is_mounted(bool is_mounted) noexcept { is_mounted_ = is_mounted; }
  bool is_mounted() const noexcept { return is_mounted_; }
  bool is_master() const noexcept { return true; }
#endif
} block_fs_mount_stat;

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
 * BlockFs use to resize the whole udisk size oneline
 *
 * \param size The udisk_size must mutiple of extend_size(10G)
 *
 * \return mount growfs retcode
 */
int block_fs_resizefs(uint64_t size);

/**
 * BlockFs use to get git and build version
 *
 * \return version string
 */
const char *block_fs_get_version();

/**
 * Check blockfs whther is mounted
 * often returns true
 *
 * \return Return blockfs ready state
 */
bool block_fs_is_mounted();

/**
 * get node master role
 *
 * \return Return true or false
 */
bool block_fs_is_master();

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
 * Change work dir to given valpath
 *
 * \param valpath target work dir path
 *
 * \return dir change retcode
 */
int block_fs_chdir(const char *valpath);

/**
 * Get work dir
 *
 * \param buf Save work dir path
 *
 * \return work dir get retcode
 */
int block_fs_getwd(char *buf, int32_t size);

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
 * Open a given file
 *
 * \param valpath File absolute path
 * \param flags File open flag
 * \param mode File open mode
 *
 * \return file fd(fd)
 */
// int block_fs_open(const char *valpath, int flags, mode_t mode);
int block_fs_open(const char *valpath, int flags, ...);

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
 * File read to sepcefic pos
 *
 * \param fd File fd
 * \param buf Write buffer
 * \param len Write length
 * \param offset read offset
 *
 * \return write bytes
 */
ssize_t block_fs_pread(int fd, void *buf, size_t len, off_t offset);

/**
 * File write to sepcefic pos
 *
 * \param fd file fd
 * \param buf Write buffer
 * \param len Write length
 * \param offset write offset
 *
 * \return write bytes
 */
ssize_t block_fs_pwrite(int fd, void *buf, size_t len, off_t offset);

/**
 * Change dir or file mode
 *
 * \param valpath Dir or file path
 * \param mode Target mode
 *
 * \return chmod retcode
 */
int block_fs_chmod(const char *valpath, mode_t mode);

int block_fs_fchmod(int fd, mode_t mode);

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
 * Check access of dir or file
 *
 * \param valpath Dir or file path
 * \param amode Access mode
 *
 * \return access retcode
 */
int block_fs_access(const char *valpath, int amode);

/**
 * Symlink dir or file to another path
 *
 * \param from Source dir or file path
 * \param to Target dir or file path
 *
 * \return symlink retcode
 */
int block_fs_link(const char *from, const char *to);

/**
 * Link dir or file to another path
 *
 * \param from Source dir or file path
 * \param to Target dir or file path
 *
 * \return link retcode
 */
int block_fs_symlink(const char *from, const char *to);

/**
 * Read link of dir or file
 *
 * \param path dir or file path
 * \param buf link storage buffer
 * \param buf link storage size
 *
 * \return link retcode
 */
int block_fs_readlink(const char *path, char *buf, size_t size);

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

BLOCKFS_FILE *block_fs_fopen(const char *filename, const char *mode);
int block_fs_fflush(BLOCKFS_FILE *stream);
int block_fs_fclose(BLOCKFS_FILE *stream);
int block_fs_fputc(int c, BLOCKFS_FILE *stream);
int block_fs_fgetc(BLOCKFS_FILE *stream);
int block_fs_fputs(const char *str, BLOCKFS_FILE *stream);
char *block_fs_fgets(char *str, int n, BLOCKFS_FILE *stream);
int block_fs_fseek(BLOCKFS_FILE *stream, off_t offset, int whence);
int block_fs_fprintf(BLOCKFS_FILE *stream, const char *format, ...);
int block_fs_fscanf(BLOCKFS_FILE *stream, const char *format, ...);
int block_fs_feof(BLOCKFS_FILE *stream);
int block_fs_ferror(BLOCKFS_FILE *stream);
int block_fs_rewind(BLOCKFS_FILE *stream);
size_t block_fs_fread(void *ptr, size_t size, size_t nmemb,
                      BLOCKFS_FILE *stream);
size_t block_fs_fwrite(const void *ptr, size_t size, size_t nmemb,
                       BLOCKFS_FILE *stream);
char *block_fs_mktemp(char *template_str);
int block_fs_mkstemp(char *template_str);
BLOCKFS_FILE *block_fs_tmpfile(void);
char *block_fs_tmpnam(char *str);

// https://www.cnblogs.com/xuyh/p/3273082.htmlstruct flcok

struct block_fs_flock {
  short int l_type; /* 锁定的状态*/
  //这三个参数用于分段对文件加锁，若对整个文件加锁，则：l_whence=SEEK_SET,l_start=0,l_len=0;
  short int l_whence; /*决定l_start位置*/
  off_t l_start;      /*锁定区域的开头位置*/
  off_t l_len;        /*锁定区域的大小*/
  pid_t l_pid;        /*锁定动作的进程*/
};

int block_fs_fcntl(int fd, int cmd, ...);

ssize_t block_fs_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

// https://www.cnblogs.com/zzb-Dream-90Time/p/10114615.html
#include <sys/types.h>
struct block_fs_statvfs {
  uint32_t f_type;         /* file system type */
  unsigned long f_bsize;   /* file system block size */
  unsigned long f_frsize;  /* fragment size */
  fsblkcnt_t f_blocks;     /* size of fs in f_frsize units */
  fsblkcnt_t f_bfree;      /* # free blocks */
  fsblkcnt_t f_bavail;     /* # free blocks for unprivileged users */
  fsfilcnt_t f_files;      /* # inodes */
  fsfilcnt_t f_ffree;      /* # free inodes */
  fsfilcnt_t f_favail;     /* # free inodes for unprivileged users */
  unsigned long f_fsid;    /* file system ID */
  unsigned long f_flag;    /* mount flags */
  unsigned long f_namemax; /* maximum filename length */
};

int block_fs_statvfs(const char *path, struct block_fs_statvfs *buf);
int block_fs_fstatvfs(int fd, struct block_fs_statvfs *buf);

#if 0
void clearerr(BLOCKFS_FILE *stream);
int fflush(BLOCKFS_FILE *stream);
int fgetpos(BLOCKFS_FILE *stream, fpos_t *pos);
BLOCKFS_FILE *freopen(const char *filename, const char *mode, BLOCKFS_FILE *stream);
int fsetpos(BLOCKFS_FILE *stream, const fpos_t *pos);
long int ftell(BLOCKFS_FILE *stream);
void setbuf(BLOCKFS_FILE *stream, char *buffer);

int block_fs_io_setup(int maxevents, io_context_t *ctxp);
int block_fs_io_destroy(io_context_t ctx);
long block_fs_io_submit(aio_context_t ctx_id, long nr, struct iocb **iocbpp);
long block_fs_io_cancel(aio_context_t ctx_id, struct iocb *iocb,
                        struct io_event *result);
long block_fs_io_getevents(aio_context_t ctx_id, long min_nr, long nr,
                           struct io_event *events, struct timespec *timeout)
#endif

/**
 * BlockFS set errno
 *
 * \param e errno
 *
 * \return void
 */
void block_fs_set_errno(int e);

/**
 * BlockFS get errno
 *
 * \param voids
 *
 * \return errno
 */
int block_fs_get_errno();

// 设置blockfs的可用空间
bool block_fs_set_available_size(uint64_t available_size);
// 返回blockfs的可用空间大小
uint64_t block_fs_get_available_size();

#ifdef __cplusplus
}
#endif
#endif