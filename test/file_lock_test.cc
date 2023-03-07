#include "../lib/file_lock.h"

#include <unistd.h>

#include <thread>

#include "../lib/logging.h"

//  g++ -o  file_lock_test file_lock_test.cc ../lib/logging.cc
//  ../lib/current_thread.cc ../lib/time_stamp.cc -lpthread

using namespace udisk::blockfs;

FileRwLock lock;

void ReadTask() {
  while (true) {
    LOG(INFO) << "########### read acquire #############";
    lock.ReadLockAcquire();
    LOG(INFO) << "########### read #############";
    sleep(10);
    lock.ReadLockRelease();
    LOG(INFO) << "########### read release #############";
    sleep(1);
  }
}

void WriteTask() {
  while (true) {
    LOG(INFO) << "########### write acquire #############";
    lock.WriteLockAcquire();
    LOG(INFO) << "########### write #############";
    lock.WriteLockRelease();
    LOG(INFO) << "########### write release #############";
    sleep(2.5);
  }
}

int main(int argc, char *argv[]) {
  std::thread t1 = std::thread(ReadTask);
  std::thread t2 = std::thread(WriteTask);
  std::thread t3 = std::thread(WriteTask);
  std::thread t4 = std::thread(ReadTask);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  return 0;
}