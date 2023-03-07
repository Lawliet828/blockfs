// Copyright (c) 2020 UCloud All rights reserved.
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "block_fs.h"
#include "file_store_udisk.h"
#include "logging.h"

static void HelpInfo() {
  LOG(INFO) << "Build time    : " << __DATE__ << " " << __TIME__;
  LOG(INFO) << "Build version : " << block_fs_get_version();
  LOG(INFO) << "Run options:";
  LOG(INFO) << " -c, --config   /usr/local/mysql/blockfs/bfs.cnf";
  LOG(INFO) << " -m, --master   Run as master node";
  LOG(INFO) << " -s, --src      Input src file name";
  LOG(INFO) << " -t, --to       Input to file name";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

int main(int argc, char *argv[]) {
  const char *config_path = "/usr/local/mysql/blockfs/bfs.cnf";
  bool master = false;
  const char *file_name_src = nullptr;
  const char *file_name_to = nullptr;

  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"master", no_argument, nullptr, 'm'},
        {"src", required_argument, nullptr, 's'},
        {"to", required_argument, nullptr, 't'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "c:ms:t:vh?", longOpts, &optIndex);
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
      case 'm': {
        master = true;
      } break;
      case 's': {
        file_name_src = optarg;
      } break;
      case 't': {
        file_name_to = optarg;
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
  if (!file_name_src || !file_name_to) {
    HelpInfo();
    std::exit(0);
  }
  int64_t ret = FileStore::Instance()->MountFileSystem(config_path, master);
  if (ret < 0) {
    return -1;
  }

  ret = block_fs_rename(file_name_src, file_name_to);
  if (ret < 0) {
    return -1;
  }
  LOG(INFO) << "rename file: " << file_name_src << " to " << file_name_to
            << " success";
  return ret;
}