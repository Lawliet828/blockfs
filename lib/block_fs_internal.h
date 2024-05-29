#ifndef LIB_INTERNAL_H_
#define LIB_INTERNAL_H_

#include "comm_utils.h"
#include "meta_defines.h"

namespace udisk::blockfs {

class Directory;
typedef std::shared_ptr<Directory> DirectoryPtr;

class File;
typedef std::shared_ptr<File> FilePtr;

class FileBlock;
typedef std::shared_ptr<FileBlock> FileBlockPtr;

struct BLOCKFS_DIR {
  bool inited_ = false; /* Scan directory yet */
  int32_t fd_;          /* Open directory file descriptor */
  uint32_t index_ = 0;  /* Current index into the entry */
  DirectoryPtr dir_;    /* Directory block */
  uint64_t size_;       /* Total valid data in the block  */
  std::mutex mutex_;
  /* Entry item `dir_' corresponds to */
  std::vector<block_fs_dirent *> dir_items_;

  int32_t fd() const noexcept { return fd_; }
  uint32_t entry_num() const noexcept { return dir_items_.size(); }
  void AddItem(block_fs_dirent *info) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    dir_items_.push_back(info);
  }
  void ClearItem() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    dir_items_.clear();
  }

  uint32_t ItemSize() const noexcept { return dir_items_.size(); }
};

}
#endif