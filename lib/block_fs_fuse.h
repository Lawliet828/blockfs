// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_BLOCK_FS_MOUNT_H_
#define LIB_BLOCK_FS_MOUNT_H_

#include "block_fs_config.h"

namespace udisk {
namespace blockfs {

void block_fs_fuse_mount(block_fs_config_info *info);
void block_fs_fuse_destroy();

}  // namespace blockfs
}  // namespace udisk
#endif