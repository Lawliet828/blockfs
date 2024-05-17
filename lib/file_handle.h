// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_HANDLE_H
#define LIB_FILE_HANDLE_H

#include <functional>

#include "block_device.h"
#include "file.h"
#include "meta_handle.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Directory;
class FileBlock;
typedef std::shared_ptr<Directory> DirectoryPtr;

// guard directory in file handle mutex
typedef std::function<bool()> FileModifyCallback;

// dh + file_name
typedef std::pair<int32_t, std::string> FileNameKey;

struct FileNameKeyHash {
  std::size_t operator()(const FileNameKey &item) const {
    std::size_t h1 = std::hash<int32_t>()(item.first);
    std::size_t h2 = std::hash<std::string>()(item.second);
    return (h1 << 1) ^ h2;
  }
};

typedef std::unordered_map<int32_t, ParentFilePtr> ParentFileHandleMap;
typedef std::unordered_map<int32_t, FilePtr> FileHandleMap;
typedef std::unordered_map<int32_t, OpenFilePtr> OpenFileHandleMap;
typedef std::unordered_map<FileNameKey, FilePtr, FileNameKeyHash> FileNameMap;

class FileHandle : public MetaHandle {
 private:
  std::list<int32_t> free_metas_;  // 文件Meta的空闲链表

  FileNameMap deleted_files_;         // 待删除的目录
  FileNameMap created_files_;         // 已创建的文件
  FileHandleMap created_fhs_;         // 已创建的文件
  OpenFileHandleMap open_files_;      // 已打开的文件
  ParentFileHandleMap parent_files_;  // 继承的父文件

 private:
  bool TransformPath(const std::string &filename, std::string &new_dirname,
                     std::string &new_filename);
  FilePtr GetCreatedFile(int32_t dh, const std::string &filename) const;

  FilePtr NewFreeFileNolock(int32_t dh, const std::string &filename);
  FilePtr NewFreeTmpFileNolock(int32_t dh);
  bool NewFile(const std::string &dirname, const std::string &filename,
               bool tmpfile, std::pair<DirectoryPtr, FilePtr> *dirs);

  bool FindFile(const std::string &dirname, const std::string &filename,
                std::pair<DirectoryPtr, FilePtr> *dirs);

  void AddOpenFile(int32_t fd, const OpenFilePtr &file) noexcept;
  void RemoveOpenFile(int32_t fd, const OpenFilePtr &file) noexcept;

 public:
  FileHandle() = default;
  ~FileHandle() = default;

  bool RunInMetaGuard(const FileModifyCallback &cb);

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;

  const uint32_t free_meta_size() const { return free_metas_.size(); }

  uint32_t PageAlignIndex(uint32_t index);

  FileMeta *NewFreeFileMeta(int32_t dh, const std::string &filename);
  void AddFileName(const FilePtr &file) noexcept;
  void AddFileHandle(const FilePtr &file) noexcept;

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
  bool RemoveParentFile(int32_t fh);
  bool RemoveParentFile(const ParentFilePtr &parent);

  bool CheckFileExist(const std::string &path);
  const OpenFilePtr &GetOpenFile(int32_t fd);
  const OpenFilePtr &GetOpenFileNolock(int32_t fd);
  const FilePtr &GetCreatedFile(int32_t fh);
  const FilePtr &GetCreatedFileNoLock(int32_t fh);
  const FilePtr &GetCreatedFile(const std::string &filename);
  const FilePtr &GetCreatedFileNoLock(const FileNameKey &key);
  const FilePtr &GetCreatedFileNolock(int32_t dh, const std::string &filename);

  FilePtr CreateFile(const std::string &filename, mode_t mode,
                     bool tmpfile = false);
  int UnlinkFile(const int32_t fh);
  int UnlinkFileNolock(const int32_t fh);

 public:
  bool UpdateMeta() override;
  int unlink(const std::string &filename);
  int mkstemp(char *template_str);

  int open(const std::string &filename, int32_t flags, mode_t mode = 0);
  int close(int32_t fh) noexcept;
  int dup(int oldfd);
  int fsync(int32_t fh);
};
}  // namespace blockfs
}  // namespace udisk
#endif