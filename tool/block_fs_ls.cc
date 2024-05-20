// Copyright (c) 2020 UCloud All rights reserved.
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>

#include "file_store_udisk.h"
#include "logging.h"

using namespace udisk::blockfs;

static void HelpInfo() {
  LOG(INFO) << "Build time    : " << __DATE__ << " " << __TIME__;
  LOG(INFO) << "Run options:";
  LOG(INFO) << " -c, --config   /data/blockfs/conf/bfs.cnf";
  LOG(INFO) << " -n, --name     Input dir name";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

static std::string GetListTimeString(time_t seconds) {
  static std::string kMonthes[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };
  struct tm tm_time;
  ::localtime_r(&seconds, &tm_time);
  std::stringstream atime_str;
  atime_str << std::setw(3) << kMonthes[tm_time.tm_mon] << " " << std::setw(2)
            << std::setfill('0') << tm_time.tm_mday << " " << std::setw(2)
            << std::setfill('0') << tm_time.tm_hour << ":" << std::setw(2)
            << std::setfill('0') << tm_time.tm_min;

  return atime_str.str();
}

int ListWorkingDirectory(const char *dirname) {
  BLOCKFS_DIR *d;
  blockfs_dirent *dir_info;

  std::string directory;
  if (!dirname) {
    directory = FileStore::Instance()->mount_config()->uxdb_mount_point_;
  } else {
    directory = dirname;
  }

  if ((d = block_fs_opendir(directory.c_str())) == nullptr) {
    return -1;
  }
  // https://blog.csdn.net/weixin_37998647/article/details/79217027
  std::cout << "total " << d->entry_num() << std::endl;
  while ((dir_info = block_fs_readdir(d)) != nullptr) {
    if (dir_info->d_type == DT_DIR) {
      std::cout << "drwxrwxr-x"
                << " " << dir_info->d_link << " "
                << "root"
                << " "
                << "root"
                << " " << std::setw(8) << dir_info->d_reclen << " "
                << GetListTimeString(dir_info->d_time_) << " "
                << dir_info->d_name << "/" << std::endl;
    } else {
      std::cout << "-rwxrwxr-x"
                << " " << dir_info->d_link << " "
                << "root"
                << " "
                << "root"
                << " " << std::setw(8) << dir_info->d_reclen << " "
                << GetListTimeString(dir_info->d_time_) << " "
                << dir_info->d_name << std::endl;
    }
  }

  block_fs_closedir(d);
  return 0;
}

int main(int argc, char *argv[]) {
  const char *config_path = "/data/blockfs/conf/bfs.cnf";
  const char *file_name = nullptr;

  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"name", required_argument, nullptr, 'n'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "c:mn:vh?", longOpts, &optIndex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c': {
        config_path = optarg;
        if (!config_path) {
          HelpInfo();
          std::exit(1);
        }
      } break;
      case 'n': {
        file_name = optarg;
      } break;
      case 'v':
      case 'h':
      case '?':
      default: {
        HelpInfo();
        std::exit(0);
      }
    }
  }
  int64_t ret = FileStore::Instance()->MountFileSystem(config_path);
  if (ret < 0) {
    return -1;
  }

  ret = ListWorkingDirectory(file_name);
  if (ret < 0) {
    LOG(ERROR) << "failed to list dir:" << file_name << ", error:" << errno;
    return -1;
  }
  // LOG(INFO) << "list file: " << file_name << " success";
  return 0;
}