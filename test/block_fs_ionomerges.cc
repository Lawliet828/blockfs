// Copyright (c) 2020 UCloud All rights reserved.
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <thread>

#include <block_fs.h>
#include "file_store_udisk.h"
#include "logging.h"
using namespace udisk::blockfs;

struct IoRequest {
  char *write_buffer_;
  uint64_t write_size_;
  uint64_t udisk_offset_;
};

class IoThread {
 private:
  bool started_ = true;
  BlockDevice *dev_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::deque<IoRequest> io_tasks_;
  std::string name_;
  std::thread thread_;
  void ThreadTask() {
    int64_t ret;
    while (started_) {
      if (!io_tasks_.empty()) {
        LOG(INFO) << "Thread name: " << name_
                  << " io task size: " << io_tasks_.size();
        if (io_tasks_.size() > 20) {
          LOG(INFO) << "Thread name: " << name_
                    << " io task size: " << io_tasks_.size();
        }
        IoRequest io = io_tasks_.front();
        io_tasks_.pop_front();
        ret = dev_->PwriteDirect(io.write_buffer_, io.write_size_,
                                 io.udisk_offset_);
        LOG(INFO) << "Wirte buffer offset: " << io.udisk_offset_
                  << " size: " << io.write_size_ << " ret: " << ret
                  << " errno: " << errno;
        assert(ret > 0);
      } else {
        LOG(INFO) << "Thread name: " << name_ << " Wait for io task";
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return (!io_tasks_.empty()); });
        lock.unlock();
      }
    }
  }

 public:
  //  https://www.runoob.com/w3cnote/cpp-std-thread.html
  IoThread(BlockDevice *dev)
      : dev_(dev), thread_(std::bind(&IoThread::ThreadTask, this)) {}
  void StartTask() { thread_.detach(); }
  void AddIoReguest(const IoRequest &task) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG(INFO) << "Add io to thread: " << name_;
    io_tasks_.push_back(std::move(task));
    cond_.notify_one();
  }
};

class BlockFsTest {
 private:
  BlockDevice *dev_;
  uint64_t run_io_num_ = 0;
  uint32_t io_thread_num_ = 8;
  std::vector<IoThread *> io_threads_;

 public:
  BlockFsTest();
  ~BlockFsTest();
  void DoIoNoMergeTest();
};

BlockFsTest::BlockFsTest() {}

BlockFsTest::~BlockFsTest() {}

void BlockFsTest::DoIoNoMergeTest() {
  dev_ = new BlockDevice();

  LOG(INFO) << "====> Io no merge test start";

  if (!dev_->Open("/dev/vdd")) {
    return;
  }
  for (uint32_t i = 0; i < io_thread_num_; ++i) {
    IoThread *thread = new IoThread(dev_);
    thread->StartTask();
    io_threads_.push_back(thread);
  }

  void *raw_buffer = nullptr;
  int32_t ret = ::posix_memalign(&raw_buffer, 512, 256 * 1024);
  if (ret < 0 || !raw_buffer) {
    return;
  }
  char *buffer = static_cast<char *>(raw_buffer);

  memset(buffer, '1', 2048);
  memset(buffer + 2048, '2', 2047);
  uint64_t offset = 0;
  uint64_t write_size = 512;

  while (true) {
    // 1. 写同一区域
    // ret = dev.Pwrite(buffer, 8192, 0);
    // LOG(INFO) << "Wirte buffer ret: " << ret;
    // assert(ret > 0);

    // 2. 写交叉区域
    // ret = dev.Pwrite(buffer, 8192, offset);
    // LOG(INFO) << "Wirte buffer offset: " << offset << " ret: " << ret;
    // assert(ret > 0);
    // offset += 4096;

    // 3. 写随机区域
    // ret = dev.Pwrite(buffer, 8192, offset);
    // LOG(INFO) << "Wirte buffer offset: " << offset << " ret: " << ret;
    // assert(ret > 0);
    // offset += ROUND_UP(rand()%16384, 512);

    // 4. 写连续区域
    // ret = dev.Pwrite(buffer, write_size, offset);
    if (run_io_num_ >= (UINT64_MAX - 1)) {
      LOG(INFO) << "Running io num up to max: " << (UINT64_MAX - 1)
                << " UINT64_MAX: " << UINT64_MAX;
      usleep(500);
      break;
    }
    IoRequest req = {buffer, write_size, offset};
    uint32_t id = rand() % io_thread_num_;
    io_threads_[id]->AddIoReguest(req);
    LOG(INFO) << "Running io num: " << run_io_num_++
              << " write_size: " << write_size;
    write_size += 512;
    if (write_size > 256 * 1024) {
      write_size = 512;
    }
    offset += write_size;
    if (offset > dev_->dev_size() - write_size) {
      offset = 0;
    }
    usleep(500);
  }
  LOG(INFO) << "====> Io no merge test end";
}

int main(int argc, char *argv[]) {
  BlockFsTest test = BlockFsTest();
  test.DoIoNoMergeTest();
  return 0;
}