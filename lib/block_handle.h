#pragma once

#include <shared_mutex>
#include <vector>

#include "device.h"
#include "meta_handle.h"

namespace udisk::blockfs {

// 主要管理空闲的BlockId资源
class BlockHandle : public MetaHandle {
 private:
  // 当前支持最大的block的个数
  // 总的udisk的容量减去元数据的空间后剩余的4M的个数
  uint32_t max_block_num_ = 0;
  std::unordered_set<uint32_t> block_id_pool_;
  std::vector<std::shared_mutex> block_locks_ = std::vector<std::shared_mutex>(100000); // TODO: 精确的大小

 public:
  BlockHandle() = default;
  ~BlockHandle() = default;

  const uint32_t max_block_num() const noexcept { return max_block_num_; }

  const uint64_t GetFreeBlockNum() {
    META_HANDLE_LOCK();
    return block_id_pool_.size();
  }

  // 主要用于元数据加载,不需要加锁
  bool GetSpecificBlockId(uint32_t block_id);

  bool GetFreeBlockIdLock(uint32_t block_id_num,
                          std::vector<uint32_t> *block_ids);

  bool PutFreeBlockIdLock(uint32_t block_id);
  bool PutFreeBlockIdLock(const std::vector<uint32_t> &block_ids);

  virtual bool InitializeMeta() override;

  std::vector<std::shared_mutex> &block_locks() { return block_locks_; }
  std::shared_mutex &block_lock(uint32_t block_id) {
    return block_locks_[block_id];
  }
};
}