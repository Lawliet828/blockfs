#include "block_handle.h"

#include "file_system.h"

namespace udisk::blockfs {

bool BlockHandle::InitializeMeta() {
  uint32_t new_max_block_num = FileSystem::Instance()->super_meta()->curr_block_num;
  std::lock_guard<std::mutex> lock(mutex_);
  for (uint32_t i = max_block_num_; i < new_max_block_num; ++i) {
    block_id_pool_.emplace(i);
  }
  max_block_num_ = new_max_block_num;
  return true;
}

bool BlockHandle::GetFreeBlockIdLock(uint32_t block_id_num,
                                     std::vector<uint32_t> *block_ids) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (block_id_pool_.size() < block_id_num) [[unlikely]] {
    SPDLOG_ERROR("block id not enough, left: {} wanted: {}",
                 block_id_pool_.size(), block_id_num);
    return false;
  }
  SPDLOG_INFO("current block pool size: {} apply block_id_num: {}",
              block_id_pool_.size(), block_id_num);
  block_ids->clear();
  auto iter = block_id_pool_.begin();
  for (uint32_t i = 0; i < block_id_num; ++i) {
    SPDLOG_INFO("apply for a new file block id: {}", *iter);
    block_ids->emplace_back(*iter);
    iter = block_id_pool_.erase(iter);
  }
  SPDLOG_DEBUG("current block pool size: {}", block_id_pool_.size());
  return true;
}

bool BlockHandle::PutFreeBlockIdLock(const std::vector<uint32_t> &block_ids) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (block_ids.size() == 0) [[unlikely]] {
    SPDLOG_ERROR("block id list empty");
    return false;
  }
  SPDLOG_INFO("current block pool size: {} put block_id_num: {}",
              block_id_pool_.size(), block_ids.size());
  for (auto id : block_ids) {
    block_id_pool_.emplace(id);
    SPDLOG_DEBUG("put block id: {} to free pool done", id);
  }
  return true;
}
}