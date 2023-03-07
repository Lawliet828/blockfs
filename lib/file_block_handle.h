// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_HANDLE_H
#define LIB_BLOCK_HANDLE_H

#include "block_device.h"
#include "crc.h"
#include "file.h"
#include "file_block.h"
#include "meta_handle.h"

namespace udisk {
namespace blockfs {

typedef std::shared_ptr<FileBlock> FileBlockPtr;

class FileBlockHandle : public MetaHandle {
 private:
  std::list<int32_t> free_metas_;  // 文件块Meta的空闲链表

 public:
  FileBlockHandle();
  ~FileBlockHandle();

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;

  bool GetFileBlockLock(uint32_t file_block_num,
                        std::vector<FileBlockPtr> *file_blocks);
  void PutFileBlockLock(uint32_t index);
  void PutFileBlockNoLock(uint32_t index);
  bool PutFileBlockLock(const std::vector<FileBlockPtr> &file_blocks);
  bool PutFileBlockNoLock(const std::vector<FileBlockPtr> &file_blocks);

  const uint32_t GetFreeMetaNum() const { return free_metas_.size(); }
};
}  // namespace blockfs
}  // namespace udisk
#endif