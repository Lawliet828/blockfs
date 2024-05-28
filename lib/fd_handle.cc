#include "fd_handle.h"

#include "file_system.h"

namespace udisk::blockfs {

bool FdHandle::InitializeMeta() {
  set_max_fd_num(FileSystem::Instance()->super_meta()->max_file_num
                 << 1);
  return true;
}
}