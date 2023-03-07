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

  if (!PreHandleAllJournal()) {
    LOG(ERROR) << "prehandle all journal fail";
    return false;
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

bool JournalHandle::PreHandleAllJournal() {
  if (!block_fs_is_master()) {
    LOG(DEBUG) << "I am not master, cannot prehandle journal";
    return true;
  }
  if (journal_maps_.empty()) {
    LOG(DEBUG) << "no need to prehandle journal";
    return true;
  }

  // 从Map中seqno最小的开始遍历
  for (auto it = journal_maps_.begin(); it != journal_maps_.end(); ++it) {
    const JournalPtr &journal = it->second;
    uint64_t jh = journal->index();
    if (!journal->PreHandle()) {
      LOG(ERROR) << "prehandle journal fail, jh: " << jh 
                 << " seq: " << it->first;
      return false;
    }
    LOG(DEBUG) << "prehandle journal success, jh: " << jh;
  }
  return true;
}

bool JournalHandle::ReplayAllJournal() {
  if (!block_fs_is_master()) {
    LOG(DEBUG) << "I am not master, cannot replay journal";
    return true;
  }
  if (journal_maps_.empty()) {
    LOG(DEBUG) << "no need to replay journal";
    return true;
  }

  // 从Map中seqno最小的开始遍历
  for (auto it = journal_maps_.begin(); it != journal_maps_.end();) {
    const JournalPtr &journal = it->second;
    // Replay允许失败, 系统启动有journal分为以下几种情况
    // 1. WriteJournal和文件操作成功, 在RecycleJournal之前系统中断
    // 2. WriteJournal成功, 文件操作中途故障, 元数据可能已经被破坏
    // 3. WriteJournal后系统立即中断, 重启后Replay自己的问题导致失败
    if (!journal->Replay()) {
      LOG(WARNING) << "replay journal fail, seq: " << it->first;
    }
    uint64_t jh = journal->index();
    if (!journal->Recycle()) {
      LOG(ERROR) << "Recycle journal fail, jh: " << jh;
      return false;
    }
    AddJournal2FreeNolock(jh);
    it = journal_maps_.erase(it);
    UpdateJournalTail();
    LOG(DEBUG) << "Recycle journal success, jh: " << jh;
  }
  // 清空replay_create_dirs_
  replay_create_dirs_.clear();
  return true;
}

// 申请0 - 4095的Journal索引
JournalPtr JournalHandle::NewJournal() {
#if 0
  // 进程起来初始化状态或者没有需要回放的时候为kJournalUnusedIndex
  int32_t next_head =
      (journal_head_ + 1) %
      FileStore::Instance()->super_meta()->block_fs_journal_num_;
  if (next_head == journal_tail_) {
    LOG(ERROR) << "journal not enuough";
    return nullptr;
  }
  uint64_t offset =
      FileStore::Instance()->super_meta()->block_fs_journal_size_ * next_head;
  BlockFsJournal *meta =
      reinterpret_cast<BlockFsJournal *>(base_addr() + offset);
  ::memset(meta, 0,
           FileStore::Instance()->super_meta()->block_fs_journal_size_);
  JournalPtr journal = std::make_shared<Journal>(next_head, meta);
  return journal;
#endif
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

bool JournalHandle::WriteJournal(const BlockFsJournalType type, const FilePtr &file) {
  if (!block_fs_is_master()) {
    LOG(ERROR) << "I am not master, cannot write journal";
    return false;
  }
  // 进度: 支持create/delete/rename/expand
  if (type == BLOCKFS_JOURNAL_TRUNCATE_FILE) {
    return true;
  }
  // 临时文件不需要写日志同步给从节点
  if (file->is_temp()) {
    LOG(WARNING) << "tmp file no need to write meta: " << file->fh();
    return true;
  }

  META_HANDLE_LOCK();
  JournalPtr journal = NewJournal();
  if (!journal) {
    return false;
  }
  journal->set_type(type);
  journal->set_seq_no(available_seq_no_);
  if (type == BLOCKFS_JOURNAL_CREATE_FILE) {
    journal->set_create_file(file);
  } else if (type == BLOCKFS_JOURNAL_DELETE_FILE) {
    journal->set_delete_file(file);
  } else if (type == BLOCKFS_JOURNAL_TRUNCATE_FILE) {
    journal->set_truncate_file(file);
  } else if (type == BLOCKFS_JOURNAL_RENAME_FILE) {
    journal->set_rename_file(file);
  } else if (type == BLOCKFS_JOURNAL_EXPAND_FILE) {
    journal->set_expand_file(file);
  }

  if (!journal->WriteMeta()) {
    return false;
  }
  AddJournal(journal);

  UpdateSequenceNumber(journal);

  file->set_seq_no(journal->seq_no());

  Dump();

  return true;
}

bool JournalHandle::WriteJournal(const BlockFsJournalType type,
                                 const DirectoryPtr &dir) {
  if (!block_fs_is_master()) {
    LOG(ERROR) << "I am not master, cannot write journal";
    return false;
  }

  META_HANDLE_LOCK();
  JournalPtr journal = NewJournal();
  if (!journal) {
    return false;
  }
  journal->set_type(type);
  journal->set_seq_no(available_seq_no_);
  if (type == BLOCKFS_JOURNAL_CREATE_DIR) {
    journal->set_create_directory(dir);
  } else if (type == BLOCKFS_JOURNAL_DELETE_DIR) {
    journal->set_delete_directory(dir);
  } else {
    return true;
  }

  if (!journal->WriteMeta()) {
    return false;
  }
  AddJournal(journal);

  UpdateSequenceNumber(journal);

  dir->set_seq_no(journal->seq_no());

  Dump();

  return true;
}

bool JournalHandle::WriteJournalExForExpandFile(seq_t seq_no, 
      FileBlockPtr last_file_block, const std::vector<FileBlockPtr> &file_blocks) {
  META_HANDLE_LOCK();
  auto itor = journal_maps_.find(seq_no);
  if (unlikely(itor == journal_maps_.end())) {
    LOG(ERROR) << "journal seq_no not exist: " << seq_no;
    return false;
  }
  const JournalPtr &journal = itor->second;
  journal->set_expand_file_ex(last_file_block, file_blocks);

  if (!journal->WriteMeta()) {
    return false;
  }
  return true;
}

/**
 * 主备模式: 主节点操作完元数据后，主动回收这条Journal,
 *           如果进程异常，那么进程重启后需要先Apply后回收
 * 主从模式: 从节点收到主节点Journal消息, Apply这条日志
 *
 * \param seq_no Journal序列号
 *
 * \return true or false
 */
bool JournalHandle::ReplayJournal(uint64_t seq_no) {
  if (!block_fs_is_master()) {
    LOG(ERROR) << "I am not master, cannot recycle journal";
    return true;
  }

  META_HANDLE_LOCK();
  auto itor = journal_maps_.find(seq_no);
  if (unlikely(itor == journal_maps_.end())) {
    LOG(ERROR) << "journal seq_no not exist: " << seq_no;
    return false;
  }
  const JournalPtr &journal = itor->second;
  return journal->Replay();
}

/**
 * 主备模式: 主节点操作完元数据后，主动回收这条Journal,
 *           如果进程异常，那么进程重启后需要先Apply后回收
 * 主从模式: 主节点收到从节点的响应后, 回收这条Journal
 *
 * \param seq_no Journal序列号
 *
 * \return true or false
 */
bool JournalHandle::RecycleJournal(uint64_t seq_no) {
  if (!block_fs_is_master()) {
    LOG(ERROR) << "I am not master, cannot recycle journal";
    return false;
  }

  META_HANDLE_LOCK();
  auto itor = journal_maps_.find(seq_no);
  if (unlikely(itor == journal_maps_.end())) {
    // 进度: 支持createfile/deletefile/createdir/deletedir
    LOG(WARNING) << "journal seq_no not exist: " << seq_no;
    return true;
//    LOG(ERROR) << "journal seq_no not exist: " << seq_no;
//    return false;
  }
  const JournalPtr &journal = itor->second;
  uint64_t jh = journal->index();
  if (!journal->Recycle()) {
    return false;
  }
  RemoveJournal(seq_no, jh);
  UpdateJournalTail();
  LOG(DEBUG) << "Recycle journal success, jh: " << jh << " seq_no: " << seq_no;
  return true;
}

bool JournalHandle::ReplayAndRecycleJournal(uint64_t seq_no) {
  if (!block_fs_is_master()) {
    LOG(ERROR) << "I am not master, cannot recycle journal";
    return true;
  }

  META_HANDLE_LOCK();
  auto itor = journal_maps_.find(seq_no);
  if (unlikely(itor == journal_maps_.end())) {
    LOG(ERROR) << "Journal seq_no not exist: " << seq_no;
    return false;
  }
  const JournalPtr &journal = itor->second;
  uint64_t jh = journal->index();
  if (journal->Replay() && journal->Recycle()) {
    RemoveJournal(seq_no, jh);
    UpdateJournalTail();
    return true;
  }
  return false;
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