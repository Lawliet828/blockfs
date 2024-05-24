
#ifndef LIB_FILE_SYSTEM_H_
#define LIB_FILE_SYSTEM_H_

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>

#include "block_fs.h"
#include "block_fs_config.h"
#include "comm_utils.h"

namespace udisk::blockfs {

class Directory;
struct FileInfo {
  int64_t size_;
  uint32_t ctime_;
  uint32_t mode_;
  char name_[1024];
  char link_[1024];
};

class FileSystem {
 protected:
  bfs_config_info mount_config_;

 private:
  FileSystem(const FileSystem&) = delete;
  FileSystem &operator=(const FileSystem&) = delete;

 public:
  explicit FileSystem() = default;
  virtual ~FileSystem() = default;

  bfs_config_info* mount_config() { return &mount_config_; }

  virtual const uint64_t GetMaxSupportFileNumber() noexcept = 0;
  virtual const uint64_t GetMaxSupportBlockNumer() noexcept = 0;
  virtual const uint64_t GetFreeBlockNumber() noexcept = 0;
  virtual const uint64_t GetBlockSize() noexcept = 0;
  virtual const uint64_t GetFreeFileNumber() noexcept = 0;
  virtual const uint64_t GetMaxFileMetaSize() noexcept = 0;
  virtual const uint64_t GetMaxFileNameLength() noexcept = 0;

  virtual int32_t MountFileSystem(const std::string& config_path) = 0;
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

  // Stat
  virtual int32_t StatPath(const std::string& path, struct stat* fileinfo) = 0;
  virtual int32_t StatPath(const int32_t fd, struct stat* fileinfo) = 0;
  virtual int32_t StatVFS(const std::string& path,
                          struct statvfs* buf) = 0;
  virtual int32_t StatVFS(const int32_t fd, struct statvfs* buf) = 0;
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
  virtual int32_t TruncateFile(const std::string& filename, int64_t size) = 0;
  virtual int32_t TruncateFile(const int32_t fd, int64_t size) = 0;
  virtual int32_t PosixFallocate(int32_t fd, int64_t offset, int64_t len) = 0;
  virtual int64_t ReadFile(int32_t fd, void* buf, size_t len) = 0;
  virtual int64_t WriteFile(int32_t fd, const void* buf, size_t len) = 0;
  virtual int64_t PreadFile(int32_t fd, void* buf, size_t len,
                            off_t offset) = 0;
  virtual int64_t PwriteFile(int32_t fd, const void* buf, size_t len,
                             off_t offset) = 0;
  virtual off_t SeekFile(int32_t fd, off_t offset, int whence) = 0;
  virtual int32_t FcntlFile(int32_t fd, int32_t set_flag) = 0;
  virtual int32_t FcntlFile(int32_t fd, int16_t lock_type) = 0;
  virtual int32_t Sync() = 0;
  virtual int32_t FileSync(const int32_t fd) = 0;
  virtual int32_t FileDataSync(const int32_t fd) = 0;
  virtual int32_t FileDup(const int32_t fd) = 0;
  virtual int32_t RemovePath(const std::string& path) = 0;

  virtual void DumpFileMeta(const std::string& path) = 0;
};

}
#endif