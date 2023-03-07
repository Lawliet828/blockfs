// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_INTERNAL_H_
#define LIB_INTERNAL_H_

#include "comm_utils.h"
#include "meta_defines.h"

// https://blog.csdn.net/lee244868149/article/details/38702367?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-4.nonecase&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-4.nonecase
// https://blog.csdn.net/wogezheerne/article/details/84994476?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-10.nonecase&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-10.nonecase
struct blockfs_dirent;

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Directory;
typedef std::shared_ptr<Directory> DirectoryPtr;

class File;
typedef std::shared_ptr<File> FilePtr;

class FileBlock;
typedef std::shared_ptr<FileBlock> FileBlockPtr;

class Journal;
typedef std::shared_ptr<Journal> JournalPtr;

struct BLOCKFS_DIR_S {
  bool inited_ = false; /* Scan directory yet */
  int32_t fd_;          /* Open directory file descriptor */
  uint32_t index_ = 0;  /* Current index into the entry */
  DirectoryPtr dir_;    /* Directory block */
  uint64_t size_;       /* Total valid data in the block  */
  std::mutex mutex_;
  /* Entry item `dir_' corresponds to */
  std::vector<blockfs_dirent *> dir_items_;

  int32_t fd() const noexcept { return fd_; }
  uint32_t entry_num() const noexcept { return dir_items_.size(); }
  void AddItem(blockfs_dirent *info) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    dir_items_.push_back(info);
  }
  void ClearItem() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    dir_items_.clear();
  }

  uint32_t ItemSize() const noexcept { return dir_items_.size(); }
};

}  // namespace blockfs
}  // namespace udisk
#endif