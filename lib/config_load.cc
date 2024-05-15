// Copyright (c) 2020 UCloud All rights reserved.

#include "config_load.h"

#include "config_parser.h"
#include "logging.h"

namespace udisk {
namespace blockfs {

bool ConfigLoader::ParseConfig(block_fs_config_info *config) {
  if (config_path_.empty()) {
    LOG(ERROR) << "config path empty";
    return false;
  }
  ConfigParser ini;
  if (ini.Load(config_path_) != 0) {
    return false;
  }
  if (!ini.HasSection("common") || !ini.HasSection("bfs") ||
      !ini.HasSection("fuse")) {
    LOG(ERROR) << "configure invalid, check sections";
    return false;
  }

  if (ini.GetStringValue("common", "log_level", &config->log_level_) != 0) {
    config->log_level_ = "INFO";
  }
  LOG(DEBUG) << "log level: " << config->log_level_;
  if (ini.GetStringValue("common", "log_path", &config->log_path_) != 0) {
    config->log_path_ = "/var/log/bfs/";
  }
  LOG(DEBUG) << "log path: " << config->log_path_;

  Logger::set_min_level(Logger::LogLevelConvert(config->log_level_));

  if (ini.GetStringValue("bfs", "device_uuid", &config->device_uuid_) != 0) {
    return false;
  }
  if (ini.GetStringValue("bfs", "uxdb_mount_point",
                         &config->uxdb_mount_point_) != 0) {
    return false;
  }

  if (ini.GetBoolValue("fuse", "fuse_debug", &config->fuse_debug_) != 0) {
    config->fuse_debug_ = false;
  }
  LOG(DEBUG) << "fuse_debug: " << config->fuse_debug_;

  if (ini.GetBoolValue("fuse", "fuse_foreground", &config->fuse_foreground_) !=
      0) {
    config->fuse_foreground_ = false;
  }
  LOG(DEBUG) << "fuse_foreground: " << config->fuse_foreground_;

  if (ini.GetStringValue("fuse", "fuse_mount_point",
                         &config->fuse_mount_point_) != 0) {
    config->fuse_mount_point_ = "/data/mysql/bfs/";
  }

  LOG(DEBUG) << "fuse mount point: " << config->fuse_mount_point_;

  std::string allow_str;
  if (ini.GetStringValue("fuse", "fuse_allow_permission", &allow_str) != 0) {
    config->fuse_allow_other_ = true;
  }
  if (allow_str.find("allow_other")) {
    config->fuse_allow_other_ = true;
    config->fuse_allow_root_ = false;
  } else if (allow_str.find("allow_root")) {
    config->fuse_allow_other_ = false;
    config->fuse_allow_root_ = true;
  }
  LOG(DEBUG) << "fuse_allow_permission: " << allow_str;

  if (ini.GetBoolValue("fuse", "fuse_auto_unmount",
                       &config->fuse_auto_unmount_) != 0) {
    config->fuse_auto_unmount_ = true;
  }

  return true;
}

}  // namespace blockfs
}  // namespace udisk
