// Copyright (c) 2020 UCloud All rights reserved.

#include "config_load.h"

#include "config_parser.h"
#include "logging.h"

using namespace udisk::blockfs;

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
  if (ini.GetBoolValue("common", "agent_enable", &config->enable_agent_) != 0) {
    config->enable_agent_ = false;
  }
  if (ini.GetStringValue("common", "agent_server", &config->agent_server_) !=
      0) {
    config->agent_server_ = "/data/mysql/bfs.sock";
  }

  if (ini.GetStringValue("common", "log_level", &config->log_level_) != 0) {
    config->log_level_ = "INFO";
  }
  LOG(DEBUG) << "log level: " << config->log_level_;
  if (ini.GetStringValue("common", "log_path", &config->log_path_) != 0) {
    config->log_path_ = "/data/mysql/log/";
  }
  LOG(DEBUG) << "log path: " << config->log_path_;

  if (ini.GetStringValue("common", "log_timestamps",
                         &config->log_timestamps_) != 0) {
    config->log_timestamps_ = "INFO";
  }
  LOG(DEBUG) << "log timestamps: " << config->log_timestamps_;

  if (config->log_timestamps_ == "UTC") {
    config->log_time_utc_ = true;
  } else if (config->log_timestamps_ == "SYSTEM") {
    config->log_time_utc_ = false;
  } else {
    config->log_time_utc_ = false;
  }

  Logger::set_min_level(Logger::LogLevelConvert(config->log_level_));

  if (ini.GetStringValue("bfs", "device_uuid", &config->device_uuid_) != 0) {
    return false;
  }
  if (ini.GetStringValue("bfs", "uxdb_mount_point",
                         &config->uxdb_mount_point_) != 0) {
    return false;
  }
  if (ini.GetIntValue("bfs", "time_update_meta", &config->time_update_meta_) !=
      0) {
    config->time_update_meta_ = 3600;
  }

  if (ini.GetBoolValue("fuse", "fuse_enable", &config->fuse_enabled_) != 0) {
    config->fuse_enabled_ = false;
  }
  LOG(DEBUG) << "fuse enable: " << config->fuse_enabled_;

  if (ini.GetBoolValue("fuse", "fuse_debug", &config->fuse_debug_) != 0) {
    config->fuse_debug_ = false;
  }
  LOG(DEBUG) << "fuse_debug: " << config->fuse_debug_;

  if (ini.GetBoolValue("fuse", "fuse_foreground", &config->fuse_foreground_) !=
      0) {
    config->fuse_foreground_ = false;
  }
  LOG(DEBUG) << "fuse_foreground: " << config->fuse_foreground_;

  config->fuse_name_ = "block_fs_mount";

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

  if (ini.GetStringValue("fuse", "fuse_setuid", &config->fuse_setuid_) != 0) {
    return false;
  }
  LOG(DEBUG) << "fuse_setuid: " << config->fuse_setuid_;

  if (ini.GetStringValue("fuse", "fuse_uid", &config->fuse_uid_) != 0) {
    return false;
  }
  LOG(DEBUG) << "fuse_uid: " << config->fuse_uid_;

  if (ini.GetStringValue("fuse", "fuse_gid", &config->fuse_gid_) != 0) {
    return false;
  }
  LOG(DEBUG) << "fuse_gid: " << config->fuse_gid_;

  if (ini.GetStringValue("fuse", "fuse_umask", &config->fuse_umask_) != 0) {
    config->fuse_umask_ = "0755";
  } else {
    if (!config->fuse_umask_.empty()) {
      config->has_fuse_umask_ = true;
    }
  }
  LOG(DEBUG) << "fuse_umask: " << config->fuse_umask_
             << " enable: " << config->has_fuse_umask_;

  if (ini.GetBoolValue("fuse", "fuse_new_thread",
                       &config->fuse_new_fuse_thread_) != 0) {
    config->fuse_new_fuse_thread_ = true;
  }
  LOG(DEBUG) << "fuse new_fuse_thread: " << config->fuse_new_fuse_thread_;

  if (ini.GetBoolValue("fuse", "fuse_auto_unmount",
                       &config->fuse_auto_unmount_) != 0) {
    config->fuse_auto_unmount_ = true;
  }
  LOG(DEBUG) << "fuse_auto_unmount: " << config->fuse_new_fuse_thread_;

  return true;
}

}  // namespace blockfs
}  // namespace udisk
