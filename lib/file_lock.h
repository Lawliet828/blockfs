// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_LOCK_H_
#define LIB_FILE_LOCK_H_

#include <assert.h>

#include <condition_variable>
#include <mutex>
#include <shared_mutex>

#include "comm_utils.h"

namespace udisk {
namespace blockfs {

static const std::string kLockPathPrefix = "/var/lock/";

// Identifies a locked file.
class FileLock {
 private:
  FileLock(const FileLock &) = delete;
  FileLock &operator=(const FileLock &) = delete;
 public:
  FileLock() = default;
  FileLock(const std::string &path) : file_name_(path) {}
  virtual ~FileLock() {}
  virtual bool lock(bool create = false) { return false; }
  virtual bool unlock() { return false; }

 protected:
  int32_t fd_ = -1;
  std::string file_name_;
};

class PosixFileLock : public FileLock {
 public:
  PosixFileLock(const std::string &uuid) : FileLock(kLockPathPrefix + uuid) {}
  ~PosixFileLock();
  bool lock(bool create = false) override;
  bool unlock() override;
};

// http://blog.sina.com.cn/s/blog_48d4cf2d0100mx6w.html
// https://blog.csdn.net/mymodian9612/article/details/52794980?utm_medium=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.control&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.control
// Write-first read-write lock
class FileRwLock {
 public:
  FileRwLock() = default;
  ~FileRwLock() = default;

  void ReadLockAcquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_read_.wait(lock, [this]() -> bool {
      return (write_count_ == 0) && (wait_write_count_ == 0);
    });
    ++read_count_;
  }

  // Multiple threads/readers can read within writing at the same time.
  void ReadLockRelease() {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(read_count_ > 0);
    --read_count_;
    if ((read_count_ == 0) && wait_write_count_ > 0) {
      cond_write_.notify_one();
    }
  }

  // There can be no read/write request when want to write
  void WriteLockAcquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++wait_write_count_;
    cond_write_.wait(lock, [this]() -> bool {
      return (write_count_ == 0) && (read_count_ == 0);
    });
    assert(write_count_ == 0);
    ++write_count_;
    --wait_write_count_;
  }

  // Only one thread/writer can be allowed
  void WriteLockRelease() {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(write_count_ == 1);
    --write_count_;
    if (wait_write_count_ == 0) {
      cond_read_.notify_all();
    } else {
      cond_write_.notify_one();
    }
  }

 private:
  mutable std::mutex mutex_;
  int32_t wait_write_count_ = 0;
  int32_t write_count_ = 0;
  int32_t read_count_ = 0;
  std::condition_variable cond_write_;
  std::condition_variable cond_read_;
};

}  // namespace blockfs
}  // namespace udisk
#endif