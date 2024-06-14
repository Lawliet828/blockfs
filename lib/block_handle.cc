#include "block_handle.h"

#include "file_system.h"
#include "spdlog/spdlog.h"

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

bool BlockHandle::GetSpecificBlockId(uint32_t block_id) {
  auto iter = block_id_pool_.find(block_id);
  if (iter != block_id_pool_.end()) {
    block_id_pool_.erase(block_id);
    return true;
  } else {
    SPDLOG_CRITICAL("block id: {} cannot be in used", block_id);
    return false;
  }
}

bool BlockHandle::GetFreeBlockIdLock(uint32_t block_id_num,
                                     std::vector<uint32_t> *block_ids) {
  META_HANDLE_LOCK();
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

bool BlockHandle::PutFreeBlockIdLock(uint32_t block_id) {
  META_HANDLE_LOCK();
  block_id_pool_.emplace(block_id);
  return true;
}

bool BlockHandle::PutFreeBlockIdLock(const std::vector<uint32_t> &block_ids) {
  META_HANDLE_LOCK();
  if (block_ids.size() == 0) [[unlikely]] {
    SPDLOG_ERROR("block id list empty");
    return false;
  }
  SPDLOG_INFO("current block pool size: {} put block_id_num: {}",
              block_id_pool_.size(), block_ids.size());
  for (auto iter = block_ids.begin(); iter != block_ids.end(); ++iter) {
    block_id_pool_.emplace(*iter);
    SPDLOG_DEBUG("put block id: {} to free pool done", *iter);
  }
  return true;
}
}