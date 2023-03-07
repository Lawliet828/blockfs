// Copyright (c) 2020 UCloud All rights reserved.
#include <getopt.h>
#include <string.h>
#include <unistd.h>

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
  LOG(INFO) << " -o, --offset   Input copy offset";
  LOG(INFO) << " -l, --length   Input copy length";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

int BlockFSCopyFileRange(const char *src, const char *to, int64_t offset,
                         int64_t size) {
  bool is_src_bfs = FileStore::Instance()->super()->CheckMountPoint(src, true);
  bool is_to_bfs = FileStore::Instance()->super()->CheckMountPoint(to, true);

  int64_t ret;
  int32_t src_fd = -1;
  int32_t dst_fd = -1;
  int64_t import_size = 0;
  int64_t read_offset = 0;
  void *raw_buffer = nullptr;
  char *buffer = nullptr;

  if (is_src_bfs) {
    src_fd = block_fs_open(src, O_RDONLY, 0777);
  } else {
    src_fd = ::open(src, O_RDONLY, 0777);
  }
  if (src_fd < 0) {
    LOG(ERROR) << "failed to open file:" << src << ", error:" << errno;
    return -1;
  }
  struct stat st;
  ::memset(&st, 0, sizeof(st));

  if (is_src_bfs) {
    if (block_fs_fstat(src_fd, &st) < 0) {
      LOG(ERROR) << "failed to fstat file:" << src << ", error:" << errno;
      block_fs_close(src_fd);
      return -1;
    }
  } else {
    if (::fstat(src_fd, &st) < 0) {
      LOG(ERROR) << "failed to fstat file:" << src << ", error:" << errno;
      ::close(src_fd);
      return -1;
    }
  }

  if (offset > st.st_size) {
    LOG(ERROR) << "offset can't exceed file size: " << st.st_size;
    goto err_cp;
  }

  if (offset + size > st.st_size) {
    LOG(ERROR) << "read size exceed file size: " << st.st_size;
    import_size = st.st_size - offset;
  } else {
    LOG(ERROR) << "read file range size: " << size;
    import_size = size;
  }

  if (is_src_bfs)
    ret = block_fs_lseek(src_fd, offset, SEEK_SET);
  else
    ret = ::lseek(src_fd, offset, SEEK_SET);
  if (ret < 0) {
    goto err_cp;
  }

  static const int64_t kImportBufferSize = 16 * 1024;

  ret = ::posix_memalign(&raw_buffer, 512, kImportBufferSize);
  if (ret < 0 || !raw_buffer) {
    LOG(ERROR) << "failed to posix_memalign, size=" << kImportBufferSize;
    goto err_cp;
  }
  buffer = static_cast<char *>(raw_buffer);

  if (is_to_bfs) {
    dst_fd = block_fs_open(to, O_WRONLY | O_CREAT);
  } else {
    dst_fd = ::open(to, O_WRONLY | O_CREAT, 0777);
  }
  if (dst_fd < 0) {
    LOG(ERROR) << "failed to create file:" << to << ", error:" << errno;
    goto err_cp;
  }

  while (import_size > 0) {
    ::memset(buffer, 0, kImportBufferSize);
    if (import_size >= kImportBufferSize) {
      if (is_src_bfs) {
        ret = block_fs_read(src_fd, buffer, kImportBufferSize);
      } else {
        ret = ::read(src_fd, buffer, kImportBufferSize);
      }
      if (ret != kImportBufferSize) {
        goto err_cp;
      }

      if (is_to_bfs) {
        ret = block_fs_write(dst_fd, buffer, kImportBufferSize);
      } else {
        ret = ::write(dst_fd, buffer, kImportBufferSize);
      }
      if (ret != kImportBufferSize) {
        goto err_cp;
      }
      import_size -= kImportBufferSize;
      read_offset += kImportBufferSize;
    } else {
      if (is_src_bfs) {
        ret = block_fs_read(src_fd, buffer, import_size);
      } else {
        ret = ::read(src_fd, buffer, import_size);
      }
      if (ret != import_size) {
        goto err_cp;
      }

      if (is_to_bfs) {
        ret = block_fs_write(dst_fd, buffer, import_size);
      } else {
        ret = ::write(dst_fd, buffer, import_size);
      }
      if (ret != import_size) {
        goto err_cp;
      }

      if (is_to_bfs) {
        ret = block_fs_fsync(dst_fd);
      } else {
        ret = ::fsync(dst_fd);
      }
      if (ret < 0) {
        LOG(ERROR) << "failed to fsync file: " << to << ", error:" << errno;
        goto err_cp;
      }
      import_size = 0;
    }
  }
  if (src_fd > 0) {
    if (is_src_bfs) {
      block_fs_close(src_fd);
    } else {
      ::close(src_fd);
    }
  }
  if (is_to_bfs) {
    block_fs_close(dst_fd);
  } else {
    ::close(dst_fd);
  }
  if (raw_buffer) {
    ::free(raw_buffer);
  }
  LOG(INFO) << "copy file range: " << src << " to " << to << " success";

  return 0;

err_cp:
  if (src_fd > 0) {
    if (is_src_bfs) {
      block_fs_close(src_fd);
    } else {
      ::close(src_fd);
    }
  }
  if (is_to_bfs) {
    block_fs_close(dst_fd);
  } else {
    ::close(dst_fd);
  }
  if (raw_buffer) {
    ::free(raw_buffer);
  }
  return -1;
}

int main(int argc, char *argv[]) {
  const char *config_path = "/usr/local/mysql/blockfs/bfs.cnf";
  bool master = false;
  const char *file_name_src = nullptr;
  const char *file_name_to = nullptr;
  int64_t offset = 0, length = 0;

  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"master", no_argument, nullptr, 'm'},
        {"src", required_argument, nullptr, 's'},
        {"to", required_argument, nullptr, 't'},
        {"offset", required_argument, nullptr, 'o'},
        {"length", required_argument, nullptr, 'l'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "c:ms:t:o:l:vh?", longOpts, &optIndex);
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
  if (!file_name_src || !file_name_to || length == 0) {
    HelpInfo();
    std::exit(0);
  }
  int64_t ret = FileStore::Instance()->MountFileSystem(config_path, master);
  if (ret < 0) {
    return -1;
  }

  ret = BlockFSCopyFileRange(file_name_src, file_name_to, offset, length);
  if (ret < 0) {
    return -1;
  }
  LOG(INFO) << "copy file: " << file_name_src << " to " << file_name_to
            << " success";
  return ret;
}
