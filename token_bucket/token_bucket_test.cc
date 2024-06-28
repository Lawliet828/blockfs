#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "token_bucket.h"

const int NS_PER_SECOND = 1e9;
const int NS_PER_MS = 1e6;
const int NS_PER_US = 1e3;

// 获取当前时间戳, 单位纳秒
int64_t Now() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * NS_PER_SECOND + ts.tv_nsec;
}

int main() {
  // 限流300MB/s
  TokenBucket limiter(300 * 1024 * 1024, 300 * 1024 * 1024);
  // 每次请求256KB
  int64_t request_size = 256 * 1024;
  // 每隔约1ms发起2次请求
  // 每隔1s打印一次请求数据量
  int64_t start_time = Now();
  int64_t last_time = start_time;
  int64_t last_size = 0;
  while (true) {
    int64_t now = Now();
    if (now - last_time >= NS_PER_SECOND) {
      printf("time: %lf s, request size: %lf MB\n", (now - last_time) / 1e9,
             (last_size / 1024.0 / 1024.0));
      last_time = now;
      last_size = 0;
    }
    for (int i = 0; i < 2; ++i) {
      if (limiter.consume(request_size)) {
        last_size += request_size;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}