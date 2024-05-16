
// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_SYSTEM_H_
#define LIB_FILE_SYSTEM_H_

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>

#include "block_fs.h"
#include "block_fs_config.h"
#include "comm_utils.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Directory;

// 抽象一个文件系统相关的数据结构定义
// 测试的时候可以使用标准的posix函数

/* Filesystem space information  */
struct SpaceInfo {
  uint64_t capacity_;  /* total size of the filesystem, in bytes */
  uint64_t free_;      /* free space on the filesystem, in bytes */
  uint64_t available_; /* free space available to a non-privileged process (may
                          be equal or less than free) */
};

enum WriteMode {
  kWriteDefault,  // use write strategy specified by flag file by default
  kWriteChains,
  kWriteFanout,
};

struct WriteOptions {
  int flush_timeout;  // in ms, <= 0 means do not timeout, == 0 means do not
                      // wait
  int sync_timeout;  // in ms, <= 0 means do not timeout, == 0 means do not wait
  int close_timeout;  // in ms, <= 0 means do not timeout, == 0 means do not
                      // wait
  int replica;
  WriteMode write_mode;
  bool sync_on_close;  // flush data to disk when closing the file
  WriteOptions()
      : flush_timeout(-1),
        sync_timeout(-1),
        close_timeout(-1),
        replica(-1),
        write_mode(kWriteDefault),
        sync_on_close(false) {}
};

struct ReadOptions {
  int timeout;  // in ms, <= 0 means do not timeout, == 0 means do not wait
  ReadOptions() : timeout(-1) {}
};

struct FileInfo {
  int64_t size_;
  uint32_t ctime_;
  uint32_t mode_;
  char name_[1024];
  char link_[1024];
};

class FileSystem : public NonCopyable {
 protected:
  bfs_config_info mount_config_;
  block_fs_mount_stat mount_stat_;

 public:
  explicit FileSystem();
  virtual ~FileSystem();

  bfs_config_info* mount_config() { return &mount_config_; }

  virtual bool set_is_mounted(bool mounted) noexcept;
  virtual const bool is_mounted() noexcept;

  virtual const int64_t time_update() noexcept;

  virtual const uint64_t GetMaxSupportFileNumber() noexcept = 0;
  virtual const uint64_t GetMaxSupportDirectoryNumber() noexcept = 0;
  virtual const uint64_t GetMaxSupportBlockNumer() noexcept = 0;
  virtual const uint64_t GetMaxBlockNumber() noexcept = 0;
  virtual const uint64_t GetFreeBlockNumber() noexcept = 0;
  virtual const uint64_t GetBlockSize() noexcept = 0;
  virtual const uint64_t GetFreeFileNumber() noexcept = 0;
  virtual const uint64_t GetMaxFileMetaSize() noexcept = 0;
  virtual const uint64_t GetMaxFileNameLength() noexcept = 0;

  virtual void PrintVersion() noexcept;
  virtual const char* GetVersion() const noexcept;
  virtual int32_t MountFileSystem(const std::string& config_path) = 0;
  virtual int32_t MountGrowfs(uint64_t size) = 0;
  virtual int32_t RemountFileSystem() = 0;
  virtual int32_t UnmountFileSystem() = 0;

  // Create directory
  virtual int32_t CreateDirectory(const std::string& path) = 0;
  virtual int32_t NewDirectory(const std::string& dirname,
                               std::unique_ptr<Directory>* result) = 0;
  // List Directory
  virtual int32_t ListDirectory(const std::string& path, FileInfo** filelist,
                                int* num) = 0;
  // Delete Directory
  virtual int32_t DeleteDirectory(const std::string& path,
                                  bool recursive = false) = 0;
  // Lock Directory
  virtual int32_t LockDirectory(const std::string& path) = 0;
  // Unlock Directory
  virtual int32_t UnlockDirectory(const std::string& path) = 0;

  virtual BLOCKFS_DIR* OpenDirectory(const std::string& dirname) = 0;
  virtual block_fs_dirent* ReadDirectory(BLOCKFS_DIR* dir) = 0;
  virtual int32_t ReadDirectoryR(BLOCKFS_DIR* dir, block_fs_dirent* entry,
                                 block_fs_dirent** result) = 0;
  virtual int32_t CloseDirectory(BLOCKFS_DIR* dir) = 0;

  // Change Work Directory
  virtual int32_t ChangeWorkDirectory(const std::string& path) = 0;
  virtual int32_t GetWorkDirectory(std::string& path) = 0;
  // Du
  virtual int32_t DiskUsage(const std::string& path, int64_t* du_size) = 0;
  // Access
  virtual int32_t Access(const std::string& path, int32_t mode) = 0;
  // Stat
  virtual int32_t StatPath(const std::string& path, struct stat* fileinfo) = 0;
  virtual int32_t StatPath(const int32_t fd, struct stat* fileinfo) = 0;
  virtual int32_t StatVFS(const std::string& path,
                          struct block_fs_statvfs* buf) = 0;
  virtual int32_t StatVFS(const int32_t fd, struct block_fs_statvfs* buf) = 0;
  // GetFileSize: get real file size
  virtual int32_t GetFileSize(const std::string& path, int64_t* file_size) = 0;

  // Open file for read or write, flags: O_WRONLY or O_RDONLY
  virtual int32_t OpenFile(const std::string& path, int32_t flags,
                           int32_t mode) = 0;
  virtual int32_t CloseFile(int32_t fd) = 0;
  virtual int32_t FileExists(const std::string& path) = 0;
  virtual int32_t CreateFile(const std::string& path, mode_t mode) = 0;
  virtual int32_t DeleteFile(const std::string& path) = 0;
  virtual int32_t RenamePath(const std::string& src,
                             const std::string& target) = 0;
  // Returns 0 on success.
  virtual int32_t CopyFile(const std::string& from, const std::string& to) = 0;
  virtual int32_t TruncateFile(const std::string& filename, int64_t size) = 0;
  virtual int32_t TruncateFile(const int32_t fd, int64_t size) = 0;
  virtual int32_t PosixFallocate(int32_t fd, int64_t offset, int64_t len) = 0;
  virtual int32_t ChmodPath(const std::string& path, int32_t mode) = 0;
  virtual int32_t GetFileModificationTime(const std::string& filename,
                                          uint64_t* filemtime) = 0;
  virtual int64_t ReadFile(int32_t fd, void* buf, size_t len) = 0;
  virtual int64_t WriteFile(int32_t fd, const void* buf, size_t len) = 0;
  virtual int64_t PreadFile(int32_t fd, void* buf, size_t len,
                            off_t offset) = 0;
  virtual int64_t PwriteFile(int32_t fd, const void* buf, size_t len,
                             off_t offset) = 0;
  virtual off_t SeekFile(int32_t fd, off_t offset, int whence) = 0;
  virtual int32_t FcntlFile(int32_t fd, int32_t set_flag) = 0;
  virtual int32_t FcntlFile(int32_t fd, int16_t lock_type) = 0;
  virtual int32_t Sync();
  virtual int32_t FileSync(const int32_t fd) = 0;
  virtual int32_t FileDataSync(const int32_t fd) = 0;
  virtual int32_t FileDup(const int32_t fd) = 0;
  virtual int32_t RemovePath(const std::string& path) = 0;
  // Create symlink
  virtual int32_t Symlink(const char* oldpath, const char* newpath);
  virtual int32_t ReadLink(const char* path, char* buf, size_t size);
  // Hard Link file src to target.
  virtual int32_t LinkFile(const std::string& src, const std::string& target);
  virtual int32_t NumFileLinks(const std::string& fname, uint64_t* count);
  // Show system status
  virtual int32_t SysStat(const std::string& stat_name,
                          std::string* result) = 0;

  virtual int32_t LockFile(const std::string& fname) = 0;
  virtual int32_t UnlockFile(const std::string& fname) = 0;

  virtual int32_t GetAbsolutePath(const std::string& in_path,
                                  std::string* output_path) = 0;

  virtual void DumpFileMeta(const std::string& path) = 0;

  virtual int64_t SendFile(int32_t out_fd, int32_t in_fd, off_t* offset,
                           int64_t count);
  virtual BLOCKFS_FILE* FileOpen(const char* filename, const char* mode);
  virtual int32_t FileClose(BLOCKFS_FILE* stream);
  virtual int32_t FileFlush(BLOCKFS_FILE* stream);
  virtual int32_t FilePutc(int c, BLOCKFS_FILE* stream);
  virtual int32_t FileGetc(BLOCKFS_FILE* stream);
  virtual int32_t FilePuts(const char* str, BLOCKFS_FILE* stream);
  virtual char* FileGets(char* str, int32_t n, BLOCKFS_FILE* stream);
  virtual int32_t FileSeek(BLOCKFS_FILE* stream, int64_t offset, int whence);
  virtual int32_t FilePrintf(BLOCKFS_FILE* stream, const char* format, ...);
  virtual int32_t FileScanf(BLOCKFS_FILE* stream, const char* format, ...);
  virtual int32_t FileEof(BLOCKFS_FILE* stream);
  virtual int32_t FileError(BLOCKFS_FILE* stream);
  virtual int32_t FileRewind(BLOCKFS_FILE* stream);
  virtual int32_t FileRead(void* ptr, size_t size, size_t nmemb,
                           BLOCKFS_FILE* stream);
  virtual int32_t FileWrite(const void* ptr, size_t size, size_t nmemb,
                            BLOCKFS_FILE* stream);
  virtual int32_t MksTemp(char* template_str);
  virtual BLOCKFS_FILE* TmpFile(void);
  virtual char* MkTemp(char* template_str);
  virtual char* TmpNam(char* str);
};

}  // namespace blockfs
}  // namespace udisk
#endif