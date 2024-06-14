#pragma once

#include <new>
#include <shared_mutex>
#include <vector>
#include <unordered_set>

#include "device.h"
#include "meta_handle.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

#ifdef __cpp_lib_hardware_interference_size
  using std::hardware_constructive_interference_size;
  using std::hardware_destructive_interference_size;
#else
  // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
  constexpr std::size_t hardware_constructive_interference_size = 64;
  constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

// 主要管理空闲的BlockId资源
class BlockHandle : public MetaHandle {
 private:
  // 当前支持最大的block的个数
  // 总的udisk的容量减去元数据的空间后剩余的4M的个数
  uint32_t max_block_num_ = 0;
  std::unordered_set<uint32_t> block_id_pool_;
  struct alignas(hardware_destructive_interference_size) Mutex {
    std::shared_mutex m;
  };
  std::array<Mutex, 100000> block_locks_; // TODO: 精确的大小

 public:
  BlockHandle() = default;
  ~BlockHandle() = default;

  const uint32_t max_block_num() const noexcept { return max_block_num_; }

  const uint64_t GetFreeBlockNum() {
    std::lock_guard<std::mutex> lock(mutex_);
    return block_id_pool_.size();
  }

  // 主要用于元数据加载,不需要加锁
  bool GetSpecificBlockId(uint32_t block_id) {
    if (block_id_pool_.contains(block_id)) {
      block_id_pool_.erase(block_id);
      return true;
    } else {
      SPDLOG_CRITICAL("block id: {} cannot be in used", block_id);
      return false;
    }
  }

  bool GetFreeBlockIdLock(uint32_t block_id_num,
                          std::vector<uint32_t> *block_ids);

  bool PutFreeBlockIdLock(uint32_t block_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    block_id_pool_.emplace(block_id);
    return true;
  }
  bool PutFreeBlockIdLock(const std::vector<uint32_t> &block_ids);

  virtual bool InitializeMeta() override;

  std::shared_mutex &block_lock(uint32_t block_id) {
    return block_locks_[block_id].m;
  }
};
}