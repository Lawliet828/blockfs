#include "fd_handle.h"

#include "file_store_udisk.h"

namespace udisk::blockfs {

bool FdHandle::InitializeMeta() {
  set_max_fd_num(FileStore::Instance()->super_meta()->max_file_num
                 << 1);
  return true;
}
}