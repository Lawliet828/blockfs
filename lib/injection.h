// Copyright (c) 2021 UCloud All rights reserved.
#ifndef LIB_INJECTION_H_
#define LIB_INJECTION_H_

#include <stdint.h>

#include <string>

#include "block_fs_config.h"
#include "logging.h"

// 打桩测试
// #define STUB_ENABLE

namespace udisk {
namespace blockfs {
// by outer pb
void block_fs_stub_inject(const std::string& key, uint32_t count);
void block_fs_stub_match(const std::string& key);
void block_fs_stub_init(block_fs_config_info* info);

}  // namespace blockfs
}  // namespace udisk
#endif