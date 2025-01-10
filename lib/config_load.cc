#include "config_load.h"

#include "config_parser.h"
#include "logging.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"   // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h"  // support for user defined types
#include "spdlog/sinks/rotating_file_sink.h"

#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

namespace udisk::blockfs {

bool ConfigLoader::ParseConfig(bfs_config_info *config) {
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
    config->log_path_ = "/var/log/mfs/";
  }
  LOG(DEBUG) << "log path: " << config->log_path_;
  // Create a file rotating logger with 100 MB size max and 3 rotated files
  auto max_size = 1024 * 1024 * 100;
  auto max_files = 3;
  auto logger = spdlog::rotating_logger_mt("mfs_spdlog", "/var/log/mfs/mfs_spdlog.log", max_size, max_files);
  spdlog::set_default_logger(logger);
  spdlog::set_level(spdlog::level::debug);
  // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
  spdlog::set_pattern("%Y%m%d %H:%M:%S.%e %t %l %@ %! - %v");
  spdlog::flush_on(spdlog::level::warn);
  spdlog::flush_every(std::chrono::seconds(3));
  SPDLOG_INFO("Welcome to spdlog version {}.{}.{}!", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

  Logger::set_min_level(Logger::LogLevelConvert(config->log_level_));

  if (ini.GetStringValue("bfs", "device_uuid", &config->device_uuid_) != 0) {
    return false;
  }

  return true;
}

}
