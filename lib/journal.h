// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_JOURNAL_H_
#define LIB_JOURNAL_H_

#include "block_fs_internal.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Journal {
 private:
  int32_t index_;
  BlockFsJournal *meta_;

  bool ReplayCreateFile();
  bool ReplayDeleteFile();
  bool ReplayTruncateFile();
  bool ReplayRenameFile();
  bool ReplayExpandFile();
  bool ReplayDeleteDirectory();
  bool PreHandleTruncateFile();
  bool PreHandleRenameFile();
  bool PreHandleExpandFile();
  bool PreHandleCreateDir();

 public:
  Journal(int32_t index, BlockFsJournal *meta) : index_(index), meta_(meta) {}
  const int32_t index() const noexcept { return index_; }
  const BlockFsJournalType type() const noexcept { return meta_->type_; }
  void set_type(BlockFsJournalType type) const noexcept { meta_->type_ = type; }
  const uint64_t seq_no() const noexcept { return meta_->seq_no_; }
  void set_seq_no(uint64_t seq_no) const noexcept { meta_->seq_no_ = seq_no; }
  void set_create_file(const FilePtr &file) const noexcept;
  void set_delete_file(const FilePtr &file) const noexcept;
  void set_expand_file(const FilePtr &file) const noexcept;
  void set_expand_file_ex(FileBlockPtr last_file_block,
                const std::vector<FileBlockPtr> &file_blocks) const;
  void set_truncate_file(const FilePtr &file) const noexcept;
  void set_rename_file(const FilePtr &file) const noexcept;
  void set_create_directory(const DirectoryPtr &dir) const noexcept;
  void set_delete_directory(const DirectoryPtr &dir) const noexcept;

 public:
  bool WriteMeta();
  bool Replay();
  bool PreHandle();
  bool Recycle();
  virtual int check() { return 0; };   // check if journal appears valid
  virtual int create() { return 0; };  // create a fresh journal
  virtual int open(uint64_t seq) { return 0; };  // open an existing journal
  virtual void close(){};                        // close an open journal
  virtual void flush(){};                        // write journal to udisk
  virtual int dump(std::ostream &out) { return -EOPNOTSUPP; }
};
}  // namespace blockfs
}  // namespace udisk
#endif