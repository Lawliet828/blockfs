#pragma once

#include <functional>
#include <list>

#include "device.h"
#include "file.h"
#include "meta_handle.h"

namespace udisk::blockfs {

class Directory;
class FileBlock;
typedef std::shared_ptr<Directory> DirectoryPtr;

// dh + file_name
typedef std::pair<ino_t, std::string> FileNameKey;

struct FileNameKeyHash {
  std::size_t operator()(const FileNameKey &item) const {
    std::size_t h1 = std::hash<int32_t>()(item.first);
    std::size_t h2 = std::hash<std::string>()(item.second);
    return (h1 << 1) ^ h2;
  }
};

typedef std::unordered_map<int32_t, ParentFilePtr> ParentFileHandleMap;
typedef std::unordered_map<FileNameKey, FilePtr, FileNameKeyHash> FileNameMap;

class FileHandle : public MetaHandle {
 private:
  std::list<ino_t> free_fhs_;  // 文件Meta的空闲链表

  FileNameMap deleted_files_;                            // 待删除的目录
  FileNameMap created_files_;                            // 已创建的文件
  std::unordered_map<ino_t, FilePtr> created_fhs_;     // 已创建的文件
  std::unordered_map<ino_t, OpenFilePtr> open_files_;  // 已打开的文件
  ParentFileHandleMap parent_files_;                     // 继承的父文件

 private:
  bool TransformPath(const std::string &filename, std::string &new_dirname,
                     std::string &new_filename);

  FilePtr NewFreeFileNolock(int32_t dh, const std::string &filename);
  FilePtr NewFreeTmpFileNolock(int32_t dh);
  bool NewFile(const std::string &dirname, const std::string &filename,
               bool tmpfile, std::pair<DirectoryPtr, FilePtr> *dirs);

  bool FindFile(const std::string &dirname, const std::string &filename,
                std::pair<DirectoryPtr, FilePtr> *dirs);

  void AddOpenFile(ino_t fd, const OpenFilePtr &file) noexcept {
    std::lock_guard lock(mutex_);
    open_files_[fd] = file;
  }
  void RemoveOpenFile(ino_t fd, const OpenFilePtr &file) noexcept {
    std::lock_guard lock(mutex_);
    open_files_.erase(fd);
  }

 public:
  FileHandle() = default;
  ~FileHandle() = default;

  // guard directory in file handle mutex
  typedef std::function<bool()> FileModifyCallback;
  bool RunInMetaGuard(const FileModifyCallback &cb) {
    META_HANDLE_LOCK();
    return cb();
  }

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;

  uint32_t free_meta_size() const { return free_fhs_.size(); }

  uint32_t PageAlignIndex(uint32_t index);

  FileMeta *NewFreeFileMeta(int32_t dh, const std::string &filename);

  void AddFileNoLock(const FilePtr &file) noexcept;
  bool RemoveFileNoLock(const FilePtr &file) noexcept;

  void AddFileToDirectory(const DirectoryPtr &parent, const FilePtr &file);
  bool RemoveFileFromoDirectory(const DirectoryPtr &parent,
                                const FilePtr &file);
  bool RemoveFileFromoDirectoryNolock(const DirectoryPtr &parent,
                                      const FilePtr &file);
  void AddFile2FreeNolock(int32_t index) noexcept;
  void AddFile2Free(int32_t index);

  bool AddParentFile(const ParentFilePtr &parent);
  bool RemoveParentFile(ino_t fh);

  bool CheckFileExist(const std::string &path);
  const OpenFilePtr &GetOpenFile(ino_t fd);
  const OpenFilePtr &GetOpenFileNolock(ino_t fd);
  const FilePtr &GetCreatedFile(int32_t fh);
  const FilePtr &GetCreatedFileNoLock(ino_t fh);
  const FilePtr &GetCreatedFile(const std::string &filename);
  const FilePtr &GetCreatedFileNoLock(const FileNameKey &key);
  const FilePtr &GetCreatedFileNolock(int32_t dh, const std::string &filename);

  FilePtr CreateFile(const std::string &filename, mode_t mode,
                     bool tmpfile = false);
  int UnlinkFileNolock(const ino_t fh);

 public:
  bool UpdateMeta() override;
  int unlink(const std::string &filename);

  int open(const std::string &filename, int32_t flags, mode_t mode = 0);
  int close(ino_t fh) noexcept;
  int dup(int oldfd);
  int fsync(ino_t fh);
};
}