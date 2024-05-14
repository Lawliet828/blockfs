// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_FS_CONFIG_H_
#define LIB_BLOCK_FS_CONFIG_H_

#ifndef HAVE_FUSE3
#define HAVE_FUSE3
#endif

#ifdef HAVE_FUSE3
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 35
#endif
#else
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif
#endif

#include <string>
#include <vector>

namespace udisk {
namespace blockfs {

typedef struct block_fs_config_info_t {
  std::string bfs_version_;
  bool enable_coredump_ = true;
  std::string log_level_;
  std::string log_path_;
  std::string log_timestamps_;
  bool log_time_utc_ = false;
  bool enable_agent_ = false;
  std::string agent_server_;

  std::string device_uuid_;
  std::string uxdb_mount_point_;
  int time_update_meta_ = 3600;

  // mount on fuse
  bool fuse_enabled_ = false;

  std::string fuse_name_;
  std::string fuse_mount_point_;

  // bfs main loop not blocked
  bool fuse_new_fuse_thread_ = true;

  /* -d: enable debug output (implies -f) */
  bool fuse_debug_ = false;
  /* -f: foreground operation */
  bool fuse_foreground_ = false;
  /* -s: disable multi-threaded operation */
  bool fuse_multi_threaded_ = false;
  /* -o clone_fd:
   * use separate fuse device fd for each thread (may improve performance)*/
  bool fuse_clone_fd_ = false;
  /* -o max_idle_threads:
   * the maximum number of idle worker threads allowed (default: 10) */
  int fuse_max_idle_threads_ = 10;
  /* -o kernel_cache: cache files in kernel */
  bool fuse_kernel_cache_ = false;
  /* -o [no]auto_cache:
   * enable caching based on modification times (off) */
  bool fuse_auto_cache_ = false;
  /* -o umask=M: set file permissions (octal) */
  bool has_fuse_umask_ = false;
  std::string fuse_umask_ = "0755";
  /* -o uid=N: set file owner */
  std::string fuse_setuid_;
  std::string fuse_uid_;
  /* -o gid=N: set file group*/
  std::string fuse_gid_;
  /* -o entry_timeout=T: cache timeout for names (1.0s) */
  double fuse_entry_timeout_ = 0.5;
  /* -o negative_timeout=T: cache timeout for deleted names (0.0s)*/
  double fuse_negative_timeout_ = 120;
  /* -o attr_timeout=T: cache timeout for attributes (1.0s)*/
  double fuse_attr_timeout_ = 0.5;
  /* -o noforget: never forget cached inodes*/
  bool fuse_noforget_ = false;
  /* -o remember=T: remember cached inodes for T seconds(0s) */
  double fuse_remember_ = 0;
  /* -o modules=M1[:M2...]:
   * names of modules to push onto filesystem stack */
  std::vector<std::string> fuse_modules_;
  /* -o allow_other:  allow access by all users */
  bool fuse_allow_other_ = true;
  /* -o allow_root: allow access by root */
  bool fuse_allow_root_ = false;
  /* -o auto_unmount: auto unmount on process termination */
  bool fuse_auto_unmount_ = true;
#ifndef HAVE_FUSE3
  bool fuse_direct_io_ = true;
  bool fuse_nonempty_ = true;
#endif
} block_fs_config_info;

}  // namespace blockfs
}  // namespace udisk
#endif