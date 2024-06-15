#pragma once

#include <dirent.h>

#include <functional>

#include "directory.h"
#include "file_handle.h"
#include "meta_handle.h"

namespace udisk::blockfs {

// guard directory in directory handle mutex
typedef std::function<bool()> DirectoryCallback;

class DirHandle : public MetaHandle {
 private:
  std::list<dh_t> free_dhs_;  // 目录Meta的空闲链表

  typedef std::unordered_map<std::string, DirectoryPtr> DirectoryNameMap;
  DirectoryNameMap created_dirs_;   // 已创建的目录
  std::unordered_map<dh_t, DirectoryPtr> created_dhs_;  // 文件句柄名字映射
  std::unordered_map<dh_t, DirectoryPtr> open_dirs_;    // 已打开的目录

 private:
  bool TransformPath(const std::string &path, std::string &dir_name);

  bool IsMountPoint(const std::string &dirname) const noexcept;
  bool CheckFileExist(const std::string &dirname,
                      bool check_parent = true) const noexcept;
  DirectoryPtr NewFreeDirectoryNolock(const std::string &dirname);
  bool NewDirectory(const std::string &dirname,
                    std::pair<DirectoryPtr, DirectoryPtr> *dirs);
  bool AddDirectory2CreateNolock(const DirectoryPtr &child);
  bool AddDirectory(const DirectoryPtr &parent, const DirectoryPtr &child);
  bool FindDirectory(const std::string &dirname,
                     std::pair<DirectoryPtr, DirectoryPtr> *dirs);
  bool RemoveDirectoryFromCreateNolock(const DirectoryPtr &child);
  bool RemoveDirectory(const DirectoryPtr &parent, const DirectoryPtr &child);

 public:
  DirHandle() = default;
  ~DirHandle() = default;

  const uint32_t GetFreeMetaSize() const noexcept { return free_dhs_.size(); }

  const DirectoryPtr &GetOpenDirectory(ino_t fd);
  const DirectoryPtr &GetCreatedDirectory(dh_t dh);
  const DirectoryPtr &GetCreatedDirectoryNolock(dh_t dh);
  const DirectoryPtr &GetCreatedDirectory(const std::string &dirname);
  const DirectoryPtr &GetCreatedDirectoryNolock(const std::string &dirname);
  int32_t DeleteDirectory(const dh_t dh);
  int32_t DeleteDirectoryNolock(const dh_t dh);

  uint32_t PageAlignIndex(uint32_t index) const;

  bool RunInMetaGuard(const DirectoryCallback &cb) {
    std::lock_guard lock(mutex_);
    return cb();
  }

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump(const std::string &path) noexcept override;

  int32_t CreateDirectory(const std::string &path);
  int32_t DeleteDirectory(const std::string &path, bool recursive);
  int32_t RenameDirectory(const std::string &from, const std::string &to);

  BLOCKFS_DIR *OpenDirectory(const std::string &path);
  block_fs_dirent *ReadDirectory(BLOCKFS_DIR *dir);
  int32_t CloseDirectory(BLOCKFS_DIR *dir);
};

}