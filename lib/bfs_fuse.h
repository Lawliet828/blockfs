#ifndef LIB_BLOCK_FS_MOUNT_H_
#define LIB_BLOCK_FS_MOUNT_H_

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)

#include <string>

namespace udisk::blockfs {

struct bfs_config_info {
  std::string log_level_;
  std::string log_path_;

  std::string device;
  std::string device_uuid_;

  std::string fuse_mount_point;

  /* -d: enable debug output (implies -f) */
  bool fuse_debug_ = false;
};

void block_fs_fuse_mount(bfs_config_info *info);

}
#endif