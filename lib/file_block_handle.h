// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_HANDLE_H
#define LIB_BLOCK_HANDLE_H

#include "block_device.h"
#include "crc.h"
#include "file.h"
#include "file_block.h"
#include "logging.h"
#include "meta_handle.h"

namespace udisk::blockfs {

class FileBlockHandle : public MetaHandle {
 private:
  std::list<int32_t> free_fbhs_;  // 文件块Meta的空闲链表

 public:
  FileBlockHandle() = default;
  ~FileBlockHandle() = default;

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;

  FileBlockPtr GetFileBlockLock();

  void PutFileBlockLock(uint32_t index);
  void PutFileBlockNoLock(uint32_t index);
  bool PutFileBlockLock(const FileBlockPtr &file_block) {
    META_HANDLE_LOCK();
    free_fbhs_.push_back(file_block->index());
    return true;
  }
};
}
#endif