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
  LOG(INFO) << " -n, --name     Input file name";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

int main(int argc, char *argv[]) {
  const char *config_path = "/usr/local/mysql/blockfs/bfs.cnf";
  bool master = false;
  const char *file_name = nullptr;

  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"master", no_argument, nullptr, 'm'},
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
      case 'm': {
        master = true;
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
  int64_t ret = FileStore::Instance()->MountFileSystem(config_path, master);
  if (ret < 0) {
    return -1;
  }

  int64_t src_fd = block_fs_open(file_name, 0);
  if (src_fd < 0) {
    LOG(ERROR) << "failed to open file:" << file_name << ", error:" << errno;
    return -1;
  }
  struct stat st;
  ::memset(&st, 0, sizeof(st));
  if (block_fs_fstat(src_fd, &st) < 0) {
    return -1;
  }
  int64_t export_size = st.st_size;

  LOG(DEBUG) << "export size: " << export_size;

  static const int64_t kExportBufferSize = 16 * 1024;

  char buffer[kExportBufferSize + 1] = {0};

  while (export_size > 0) {
    ::memset(buffer, 0, kExportBufferSize);
    if (export_size >= kExportBufferSize) {
      ret = block_fs_read(src_fd, buffer, kExportBufferSize);
      if (ret != kExportBufferSize) {
        LOG(ERROR) << "read size not matched, ret: " << ret
                   << " wanted: " << kExportBufferSize;
        return -1;
      }
      DebugHexBuffer(buffer, kExportBufferSize);
      export_size -= kExportBufferSize;
    } else {
      ret = block_fs_read(src_fd, buffer, export_size);
      if (ret != export_size) {
        LOG(ERROR) << "read size not matched, ret: " << ret
                   << " wanted: " << export_size;
        return -1;
      }
      DebugHexBuffer(buffer, export_size);
      export_size = 0;
    }
  }
  LOG(INFO) << "cat file: " << file_name << " success";
  return 0;
}