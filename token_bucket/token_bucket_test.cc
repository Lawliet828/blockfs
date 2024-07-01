#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "token_bucket.h"

int main() {
  DynamicTokenBucket bw_token_bucket;
  // 限流300MB/s
  double kbps = 300 * 1024;
  // 每次请求256KB
  int64_t request_size = 256;
  double wait_time = 0.0;

  // 每隔约1ms发起10次请求
  // 每隔1s打印一次请求数据量
  auto duration = std::chrono::steady_clock::now().time_since_epoch();
  double last_time = std::chrono::duration<double>(duration).count();
  int64_t last_size = 0;
  while (true) {
    auto duration = std::chrono::steady_clock::now().time_since_epoch();
    double now = std::chrono::duration<double>(duration).count();

    if (now - last_time >= 1.0) {
      printf("time: %lf s, request size: %lf MB\n", (now - last_time),
             (last_size / 1024.0));
      last_time = now;
      last_size = 0;
    }

    if (wait_time > now) {
      // printf("wait time: %lf\n", wait_time - now);
    } else {
      int64_t to_comsume_kb = request_size * 10;
      std::optional<double> bw_ret = bw_token_bucket.consumeWithBorrowNonBlocking(to_comsume_kb, kbps, kbps);
      last_size += to_comsume_kb;
      double nap_time = bw_ret.value_or(0.0);
      wait_time = nap_time + now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}