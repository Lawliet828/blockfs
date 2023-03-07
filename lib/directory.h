// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_DIR_H_
#define LIB_DIR_H_

#include "block_device.h"
#include "block_fs.h"
#include "file_handle.h"
#include "inode.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

class Directory;
typedef std::shared_ptr<Directory> DirectoryPtr;

class Directory : public Inode<DirMeta, File>,
                  public std::enable_shared_from_this<Directory> {
 private:
  int32_t nlink_ = 0;
  uint32_t ack_cnt_ = 0;
  std::unordered_map<dh_t, DirectoryPtr> child_dir_maps_;

 private:
  void ClearMeta() const noexcept;

 public:
  Directory() = default;
  Directory(DirMeta *meta) : Inode(meta) {}
  virtual ~Directory() {}

  void set_dir_name(const std::string &to) {
    ::memset(meta_->dir_name_, 0, sizeof(meta_->dir_name_));
    ::memcpy(meta_->dir_name_, to.c_str(), sizeof(meta_->dir_name_));
  }
  std::string dir_name() const { return std::string(meta_->dir_name_); }
  void set_dh(dh_t dh) const noexcept { meta_->dh_ = dh; }
  const dh_t dh() const { return meta_->dh_; }
  void set_used(bool used) const noexcept { meta_->used_ = used; }
  const uint64_t seq_no() const { return meta_->seq_no_; }
  void set_seq_no(uint64_t seq_no) const noexcept { meta_->seq_no_ = seq_no; }
  const uint32_t ChildCount() noexcept;
  const uint32_t DecAckCnt() { return --ack_cnt_; }
  void IncLinkCount() noexcept;
  void DecLinkCount() noexcept;
  const uint32_t LinkCount() noexcept;

  void set_atime(time_t time) noexcept override { meta_->atime_ = time; }
  time_t atime() noexcept override { return meta_->atime_; }
  void set_mtime(time_t time) noexcept override { meta_->mtime_ = time; }
  time_t mtime() noexcept override { return meta_->mtime_; }
  void set_ctime(time_t time) noexcept override { meta_->ctime_ = time; }
  time_t ctime() noexcept override { return meta_->ctime_; }

  bool Suicide();
  bool SuicideNolock();

  void ScanDir(std::vector<blockfs_dirent *> &dir_infos);

  bool ForceRemoveAllFiles();
  bool AddChildFile(const FilePtr &file);
  bool AddChildFileNoLock(const FilePtr &file);
  bool RemoveChildFile(const FilePtr &file);
  bool RemoveChildFileNolock(const FilePtr &file);
  bool AddChildDirectoryNolock(const DirectoryPtr &child);
  bool AddChildDirectory(const DirectoryPtr &child);
  bool RemovChildDirectory(const DirectoryPtr &child);

  // common dir/file functions
  void stat(struct stat *buf) override;
  int chmod(mode_t mode) override { return 0; }
  int chown(uid_t uid, gid_t gid) override { return 0; }
  int access(int mask) const override { return 0; }
  int rename(const std::string &to) override;
  int utimens(const timespec lastAccessTime,
              const timespec lastModificationTime) override {
    return 0;
  }
  int remove() override { return 0; }

  void DumpMeta() override;
  static void ClearMeta(dh_t dh) noexcept;
  bool WriteMeta() override;
  static bool WriteMeta(dh_t dh);
  void UpdateTimeStamp(bool a = false, bool m = false, bool c = false) override;
};
}  // namespace blockfs
}  // namespace udisk
#endif