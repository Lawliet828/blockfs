#include "block_handle.h"

#include "file_system.h"
#include "logging.h"

namespace udisk::blockfs {

bool BlockHandle::InitializeMeta() {
  set_max_block_num(FileSystem::Instance()->super_meta()->curr_block_num_);
  return true;
}

// 新增加的block id加到资源池, 总的udisk size做了限制,
// 所以传入的new_max_block_num个数肯定是受到限制的
void BlockHandle::set_max_block_num(uint32_t new_max_block_num) noexcept {
  LOG(DEBUG) << "set max block num: " << new_max_block_num;
  META_HANDLE_LOCK();
  // 如果是扩容的话, 只需要吧offset湖面的空间串联起来
  // 如果是缩容的话, 要确定缩容的offset后面的block未被使用
  if (new_max_block_num < max_block_num_) {
    LOG(ERROR) << "less than curr block num, shrink udisk not supported yet";
    return;
  }

  for (uint32_t i = max_block_num_; i < new_max_block_num; ++i) {
    block_id_pool_.emplace(i);
  }
  max_block_num_ = new_max_block_num;
}

bool BlockHandle::GetSpecificBlockId(uint32_t block_id) {
  auto iter = block_id_pool_.find(block_id);
  if (iter != block_id_pool_.end()) {
    block_id_pool_.erase(block_id);
    return true;
  } else {
    LOG(FATAL) << "block id: " << block_id << " cannot be in used";
    return false;
  }
}

bool BlockHandle::GetFreeBlockIdLock(uint32_t block_id_num,
                                     std::vector<uint32_t> *block_ids) {
  META_HANDLE_LOCK();
  if (block_id_pool_.size() < block_id_num) [[unlikely]] {
    LOG(ERROR) << "block id not enough, left: " << block_id_pool_.size()
               << " wanted: " << block_id_num;
    return false;
  }
  LOG(INFO) << "current block pool size: " << block_id_pool_.size()
            << " apply block_id_num: " << block_id_num;
  block_ids->clear();
  auto iter = block_id_pool_.begin();
  for (uint32_t i = 0; i < block_id_num; ++i) {
    LOG(INFO) << "apply for a new file block id: " << *iter;
    block_ids->emplace_back(*iter);
    iter = block_id_pool_.erase(iter);
  }
  LOG(DEBUG) << "current block pool size: " << block_id_pool_.size();
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
    LOG(ERROR) << "block id list empty";
    return false;
  }
  LOG(INFO) << "current block pool size: " << block_id_pool_.size()
            << " put block_id_num: " << block_ids.size();
  for (auto iter = block_ids.begin(); iter != block_ids.end(); ++iter) {
    block_id_pool_.emplace(*iter);
    LOG(DEBUG) << "put block id: " << *iter << " to free pool done";
  }
  return true;
}
}