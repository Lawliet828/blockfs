// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_FS_CONFIG_H_
#define LIB_BLOCK_FS_CONFIG_H_

#ifndef HAVE_FUSE3
#define HAVE_FUSE3
#endif

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)

#include <string>
#include <vector>

namespace udisk {
namespace blockfs {

struct block_fs_config_info {
  bool enable_coredump_ = true;
  std::string log_level_;
  std::string log_path_;

  std::string device_uuid_;
  std::string uxdb_mount_point_;

  std::string fuse_mount_point;

  /* -d: enable debug output (implies -f) */
  bool fuse_debug_ = false;
  /* -f: foreground operation */
  bool fuse_foreground_ = false;
  /* -s: disable multi-threaded operation */
  bool fuse_multi_threaded_ = false;
  /* -o max_idle_threads:
   * the maximum number of idle worker threads allowed (default: 10) */
  int fuse_max_idle_threads_ = 10;
  /* -o kernel_cache: cache files in kernel */
  bool fuse_kernel_cache_ = false;
  /* -o entry_timeout=T: cache timeout for names (1.0s) */
  double fuse_entry_timeout_ = 0.5;
  /* -o attr_timeout=T: cache timeout for attributes (1.0s)*/
  double fuse_attr_timeout_ = 0.5;
  /* -o auto_unmount: auto unmount on process termination */
  bool fuse_auto_unmount_ = true;
};

}  // namespace blockfs
}  // namespace udisk
#endif