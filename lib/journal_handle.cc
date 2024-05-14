#include <algorithm>

#include "crc.h"
#include "file_store_udisk.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

JournalHandle::JournalHandle() {}

JournalHandle::~JournalHandle() {}

/**
 * @brief 读取日志, 用以判断系统是否故障. 如果存在日志, 可能元数据有问题
 * 所有元数据InitializeMeta之后, 调用ReplayAllJournal
 *
 * @param void
 *
 * @return success or failed
 */
bool JournalHandle::InitializeMeta() {
  BlockFsJournal *meta;
  uint64_t journal_size = FileStore::Instance()->super_meta()->block_fs_journal_size_;
  for (uint64_t idx = 0;
       idx < FileStore::Instance()->super_meta()->block_fs_journal_num_;
       ++idx) {
    meta = reinterpret_cast<BlockFsJournal *>(
        base_addr() + journal_size * idx);
    uint32_t crc =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              journal_size - sizeof(meta->crc_));
    if (unlikely(meta->crc_ != crc)) {
      // 说明是写日志时掉电或者系统崩溃
      LOG(WARNING) << "journal [" << idx << "] crc error, read:" << meta->crc_
                 << " cal: " << crc;
      // 清除该条日志
      ::memset(meta, 0, journal_size);
      meta->jh_ = idx;
      meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                          sizeof(BlockFsJournal) - sizeof(meta->crc_));
      int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
          meta, journal_size,
          FileStore::Instance()->super_meta()->block_fs_journal_offset_ +
              journal_size * idx);
      if (unlikely(ret != static_cast<int64_t>(journal_size))) {
        LOG(ERROR) << "Recycle journal: " << idx << " failed, ret: " << ret;
        return false;
      }
    }

    if (meta->seq_no_ != kReservedUnusedSeq) {
      LOG(INFO) << "journal index: " << idx << " seqno: " << meta->seq_no_
                << " type: " << meta->type_;
      JournalPtr journal = std::make_shared<Journal>(idx, meta);
      journal_maps_[meta->seq_no_] = journal;
    } else {
      // 存入空闲链表
      AddJournal2FreeNolock(idx);
    }
  }

  uint64_t min_seq_no = 0, max_seq_no = 0;
  if (!journal_maps_.empty()) {
    min_seq_no = journal_maps_.begin()->first;
    max_seq_no = journal_maps_.rbegin()->first;
    journal_head_ = journal_maps_[max_seq_no]->index();
    journal_tail_ = journal_maps_[min_seq_no]->index();
    available_seq_no_ = max_seq_no + 1;
  }

  LOG(DEBUG) << "load journal success,"
             << " used journal num: " << journal_maps_.size()
             << " journal head_: " << journal_head_
             << " journal tail_: " << journal_tail_
             << " min_seq_no_: " << min_seq_no << " max_seq_no_: " << max_seq_no
             << " available_seq_no_: " << available_seq_no_;
  return true;
}

bool JournalHandle::FormatAllMeta() {
  uint64_t journal_total_size =
      FileStore::Instance()->super_meta()->journal_total_size_;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      journal_total_size, FileStore::Instance()->dev()->block_size());

  BlockFsJournal *meta;
  LOG(INFO) << "format journal num: "
            << FileStore::Instance()->super_meta()->block_fs_journal_num_;
  for (uint64_t i = 0;
       i < FileStore::Instance()->super_meta()->block_fs_journal_num_; ++i) {
    meta = reinterpret_cast<BlockFsJournal *>(
        buffer->data() +
        FileStore::Instance()->super_meta()->block_fs_journal_size_ * i);
    meta->seq_no_ = kReservedUnusedSeq;
    meta->jh_ = i;
    meta->crc_ =
        Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
              FileStore::Instance()->super_meta()->block_fs_journal_size_ -
                  sizeof(meta->crc_));
  }
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      buffer->data(), FileStore::Instance()->super_meta()->journal_total_size_,
      FileStore::Instance()->super_meta()->block_fs_journal_offset_);
  if (ret != static_cast<int64_t>(
                 FileStore::Instance()->super_meta()->journal_total_size_)) {
    LOG(ERROR) << "write journal error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all journal success";
  return true;
}

// 申请0 - 4095的Journal索引
JournalPtr JournalHandle::NewJournal() {
  if (unlikely(free_metas_.empty())) {
    LOG(ERROR) << "journal not enuough";
    block_fs_set_errno(ENFILE);
    return nullptr;
  }
  uint64_t jh = free_metas_.front();
  LOG(INFO) << "create new journal jh: " << jh;

  BlockFsJournal* meta = reinterpret_cast<BlockFsJournal *>(base_addr() +
    FileStore::Instance()->super_meta()->block_fs_journal_size_ * jh
    );
  if (unlikely(kReservedUnusedSeq != meta->seq_no_ || jh != meta->jh_)) {
    LOG(ERROR) << "new journal meta invalid, jh: " << meta->jh_
               << " wanted jh: " << jh;
    block_fs_set_errno(EINVAL);
    return nullptr;
  }

  JournalPtr journal = std::make_shared<Journal>(jh, meta);

  // free_meta_此处更新, journal_maps_在AddJournal中更新, 是否合适?
  free_metas_.pop_front();

  return journal;
}

bool JournalHandle::AddJournal(const JournalPtr &journal) {
  LOG(DEBUG) << "AddJournal, jh: " << journal->index()
             << " seq_no: " << journal->seq_no()
             << " type: " << journal->type();
  journal_maps_[journal->seq_no()] = journal;
  return true;
}

bool JournalHandle::RemoveJournal(uint64_t seq_no, uint64_t jh) {
  journal_maps_.erase(seq_no);
  free_metas_.push_back(jh);
  return true;
}

void JournalHandle::UpdateSequenceNumber(const JournalPtr &journal) {
  // 写日志成功后才更新可用的seqno
  journal_head_ = journal->index();
  if (kMaxUnCommittedSeq == available_seq_no_) {
    available_seq_no_ = kMinUnCommittedSeq;
  } else {
    ++available_seq_no_;
  }
}

void JournalHandle::UpdateJournalTail() {
  journal_tail_ = (journal_tail_ + 1) %
                  FileStore::Instance()->super_meta()->block_fs_journal_num_;
}

void JournalHandle::Dump() noexcept {
  LOG(INFO) << "used journal num: " << journal_maps_.size()
            << " free journal num: " << free_metas_.size();
  for (auto &item : journal_maps_) {
    JournalPtr journal = item.second;
    LOG(INFO) << "journal jh: " << journal->index() << " seq_no: " << journal->seq_no()
              << " type: " << journal->type();
  }
}

void JournalHandle::Dump(const std::string &file_name) noexcept {}

void JournalHandle::AddJournal2FreeNolock(uint64_t index) noexcept {
  free_metas_.push_back(index);
}

dh_t JournalHandle::GetReplayCreateDir(std::string dir_name) {
  auto iter = replay_create_dirs_.find(dir_name);
  if (iter == replay_create_dirs_.end()) {
    return -1;
  }
  return iter->second;
}

void JournalHandle::SetReplayCreateDir(std::string dir_name, dh_t dh) {
  replay_create_dirs_[dir_name] = dh;
}

bool JournalHandle::IsReplayingJournal() const {
  return 0 != replay_create_dirs_.size();
}

}  // namespace blockfs
}  // namespace udisk