#include "file_system.h"

#include "comm_utils.h"
#include "logging.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

/* 两次转换将宏的值转成字符串 */
#define _BFS_MACROSTR(a) (#a)
#define BFS_MACROSTR(a) (_BFS_MACROSTR(a))

FileSystem::FileSystem(/* args */) {}

FileSystem::~FileSystem() {}

int32_t FileSystem::Sync() {
  LOG(WARNING) << "sync not implemented yet";
  return -1;
}

// Hard Link file src to target.
int32_t FileSystem::LinkFile(const std::string &src,
                             const std::string &target) {
  LOG(WARNING) << "link not implemented yet";
  return -1;
}

}  // namespace blockfs
}  // namespace udisk