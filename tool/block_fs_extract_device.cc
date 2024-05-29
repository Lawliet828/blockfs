// Copyright (c) 2020 UCloud All rights reserved.
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "file_system.h"
#include "logging.h"

static void HelpInfo() {
  LOG(INFO) << "Build time    : " << __DATE__ << " " << __TIME__;
  LOG(INFO) << "Run options:";
  LOG(INFO) << " -c, --config   /usr/local/mysql/blockfs/bfs.cnf";
  LOG(INFO) << " -m, --master   Run as master node";
  LOG(INFO) << " -d, --dev      Input dev name";
  LOG(INFO) << " -n, --name     Input out file name";
  LOG(INFO) << " -o, --offset   Input copy offset";
  LOG(INFO) << " -l, --length   Input copy length";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

bool ExportBlkDevice(const char *dev, const char *file_name, int64_t offset,
                     int64_t size) {
  int fd = ::open(dev, O_RDONLY | O_LARGEFILE | O_CLOEXEC);
  if (fd < 0) {
    LOG(ERROR) << "failed to open dev: " << dev << ", error:" << errno;
    return false;
  }

  static const int64_t kExportBufferSize = 16 * 1024;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(kExportBufferSize);

  int32_t dst_fd = ::open(file_name, O_WRONLY | O_CREAT, 0777);
  if (dst_fd < 0) {
    LOG(ERROR) << "failed to open file:" << file_name << ", error:" << errno;
    return false;
  }
  if (::fallocate(dst_fd, 0, 0, size) < 0) {
    LOG(ERROR) << "failed to fallocate file: " << file_name
               << ", error:" << errno;
    ::close(dst_fd);
    return false;
  }

  int64_t ret;
  int64_t read_offset = offset;
  while (size > 0) {
    ::memset(buffer->data(), 0, kExportBufferSize);
    if (size >= kExportBufferSize) {
      ret = ::pread(fd, buffer->data(), kExportBufferSize, read_offset);
      if (ret != kExportBufferSize) {
        ::close(dst_fd);
        return false;
      }
      ret = ::pwrite(dst_fd, buffer->data(), kExportBufferSize, read_offset);
      if (ret != kExportBufferSize) {
        ::close(dst_fd);
        return false;
      }
      size -= kExportBufferSize;
      read_offset += kExportBufferSize;
    } else {
      ret = ::pread(fd, buffer->data(), size, read_offset);
      if (ret != size) {
        ::close(dst_fd);
        return false;
      }
      ret = ::pwrite(dst_fd, buffer->data(), size, read_offset);
      if (ret != size) {
        ::close(dst_fd);
        return false;
      }
      if (::fsync(dst_fd) < 0) {
        LOG(ERROR) << "failed to fsync file: " << file_name
                   << ", error:" << errno;
        ::close(dst_fd);
        return false;
      }
      size = 0;
    }
  }
  ::close(dst_fd);
  LOG(INFO) << "export: " << file_name << " success";
  return true;
}

int main(int argc, char *argv[]) {
  const char *config_path = "/usr/local/mysql/blockfs/bfs.cnf";
  const char *dev_name = nullptr;
  const char *file_name = nullptr;
  int64_t offset = 0, length = 0;

  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"master", no_argument, nullptr, 'm'},
        {"name", required_argument, nullptr, 'n'},
        {"dev", required_argument, nullptr, 'd'},
        {"offset", required_argument, nullptr, 'o'},
        {"length", required_argument, nullptr, 'l'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "c:md:n:o:l:vh?", longOpts, &optIndex);
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
      case 'd': {
        dev_name = optarg;
      } break;
      case 'n': {
        file_name = optarg;
      } break;
      case 'o': {
        offset = ::atoi(optarg);
      } break;
      case 'l': {
        length = ::atoi(optarg);
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
  if (!dev_name || !file_name || length == 0) {
    LOG(ERROR) << "dev_name: " << dev_name << " file_name: " << file_name
               << " length: " << length;
    HelpInfo();
    std::exit(0);
  }
  if (!ExportBlkDevice(dev_name, file_name, offset, length)) {
    return -1;
  }

  LOG(INFO) << "export dev " << dev_name << " success";
  return 0;
}
