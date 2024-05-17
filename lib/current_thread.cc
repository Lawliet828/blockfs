#include "current_thread.h"

#include <cxxabi.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace udisk {
namespace blockfs {

namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread const char* t_threadName = "unknown";
}  // namespace CurrentThread

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = gettid();
  }
}

}  // namespace blockfs
}  // namespace udisk