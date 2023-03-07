// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_JOURNAL_HANDLE_H
#define LIB_JOURNAL_HANDLE_H

#include "block_device.h"
#include "block_fs_internal.h"
#include "journal.h"
#include "meta_handle.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class JournalHandle : public MetaHandle {
private:
  bool sync_journal_ = true;
  int32_t journal_head_ = kJournalUnusedIndex;
  int32_t journal_tail_ = kJournalUnusedIndex;
  uint64_t available_seq_no_ = kMinUnCommittedSeq;

  std::list<uint64_t> free_metas_;  // 日志Meta的空闲链表

  // 0是保留序列号, 1-UIN64_MAX是使用的序列号,
  // 进程重启读取加载到map中, 默认键值升序排列,
  // 通过该map能找到最小最大的seqno和head,tail.
  std::map<seq_t, JournalPtr> journal_maps_;

  // 回放日志时,创建的文件夹
  std::map<std::string, dh_t> replay_create_dirs_;

private:
  JournalPtr NewJournal();
  bool AddJournal(const JournalPtr &journal);
  bool RemoveJournal(uint64_t seq_no, uint64_t jh);
  void UpdateSequenceNumber(const JournalPtr &journal);
  void UpdateJournalTail();
  bool PreHandleAllJournal();

public:
  JournalHandle();
  ~JournalHandle();

  virtual bool InitializeMeta() override;
  virtual bool FormatAllMeta() override;
  virtual void Dump() noexcept override;
  virtual void Dump(const std::string &file_name) noexcept override;

  bool WriteJournal(BlockFsJournal *buf);
  bool WriteJournal(const BlockFsJournalType type, const FilePtr &file);
  bool WriteJournal(const BlockFsJournalType type, const DirectoryPtr &dir);
  bool WriteJournalExForExpandFile(seq_t seq_no, FileBlockPtr last_file_block,
                          const std::vector<FileBlockPtr> &file_blocks);
  bool ReplayAllJournal();
  bool ReplayJournal(uint64_t seq_no);
  bool RecycleJournal(uint64_t seq_no);
  bool ReplayAndRecycleJournal(uint64_t seq_no);

  // 主备模式下采用同步写日志,操作,删日志的模式
  // 如果期间进程因为各种原因异常, 需要根据日志回放.
  void set_sync_journal(bool sync = false) { sync_journal_ = sync; }

  void AddJournal2FreeNolock(uint64_t index) noexcept;
  bool HasJournal() const { return 0 != journal_maps_.size(); }

  dh_t GetReplayCreateDir(std::string dir_name);
  void SetReplayCreateDir(std::string dir_name, dh_t dh);
  bool IsReplayingJournal() const;
};
}  // namespace blockfs
}  // namespace udisk
#endif