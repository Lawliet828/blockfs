// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_CONFIG_LOAD_H_
#define LIB_CONFIG_LOAD_H_

#include "bfs_fuse.h"

namespace udisk {
namespace blockfs {

class ConfigLoader {
 public:
  ConfigLoader(const std::string &config_path) : config_path_(config_path) {}
  ~ConfigLoader() {}

  bool ParseConfig(bfs_config_info *config);

 private:
  std::string config_path_;
};

}  // namespace blockfs
}  // namespace udisk
#endif