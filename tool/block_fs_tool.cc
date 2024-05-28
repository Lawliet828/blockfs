// Copyright (c) 2020 UCloud All rights reserved.
#include <fcntl.h>
#include <getopt.h>
#include <sys/cdefs.h>

#include <chrono>
#include <iomanip>

#include "crc.h"
#include "file_system.h"
#include "logging.h"

using namespace udisk::blockfs;

class BlockFsTool {
 private:
  enum ToolType {
    BLOCKFS_NONE,
    BLOCKFS_FORMAT,
    BLOCKFS_DUMP,
    BLOCKFS_BLK,
    BLOCKFS_FILE_META,  // file meta
  };
  ToolType tool_type_ = BLOCKFS_NONE;
  std::string dump_arg_;
  std::string dev_name_;
  std::string blk_info_;  // offset and size
  std::string file_name_;

 private:
  void HelpInfo();

 public:
  BlockFsTool() {}
  ~BlockFsTool() {}
  void ParseOption(int argc, char **argv);
  bool DoBlockFsTool();

 public:
  void PrintFileMetadata(const std::string &file_name);
  bool ExportBlkDeviceContents();
};

void BlockFsTool::ParseOption(int argc, char **argv) {
  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"device", required_argument, nullptr, 'd'},
        {"format", no_argument, nullptr, 'f'},
        {"dump", required_argument, nullptr, 'p'},
        {"meta", required_argument, nullptr, 'm'},
        {"blk", required_argument, nullptr, 'b'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "m:b:d:p:fh", longOpts, &optIndex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'd': {
        if (argc < 4) {
          LOG(ERROR) << "there is too few argument: " << argc;
          HelpInfo();
          exit(1);
        }
        dev_name_ = std::string(optarg);
        if (dev_name_.empty()) {
          LOG(ERROR) << "the device name empty";
          HelpInfo();
          exit(1);
        }
      } break;
      case 'f': {
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_FORMAT;
      } break;
      case 'p': {
        if (argc < 5) {
          LOG(ERROR) << "there is too few argument: " << argc;
          HelpInfo();
          exit(1);
        }
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_DUMP;
        dump_arg_ = std::string(optarg);
        LOG(INFO) << "dump flag: " << dump_arg_;
      } break;
      case 'b': {
        if (argc < 5) {
          LOG(ERROR) << "there is too few argument: " << argc;
          HelpInfo();
          exit(1);
        }
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_BLK;
        blk_info_ = std::string(optarg);
        LOG(INFO) << "blk_info: " << blk_info_;
      } break;
      case 'm': {
        if (argc < 5) {
          LOG(ERROR) << "there is too few argument: " << argc;
          HelpInfo();
          exit(1);
        }
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_FILE_META;
        file_name_ = std::string(optarg);
        LOG(INFO) << "file name: " << blk_info_;
      } break;
      case 'h':
      default: {
        HelpInfo();
        exit(0);
      }
    }
  }
  if (tool_type_ == BLOCKFS_NONE) {
    LOG(ERROR) << "no option exist, Please check";
    HelpInfo();
    exit(1);
  }
  return;
}

void BlockFsTool::HelpInfo() {
  LOG(INFO) << "Build time    : " << __DATE__ << " " << __TIME__;
  LOG(INFO) << "Run options:";
  LOG(INFO) << " -d, --device   Device such as: /dev/vdb";
  LOG(INFO) << " -f, --format   Format device.";
  LOG(INFO) << " -p, --dump     Dump device, xxx args.";
  LOG(INFO) << " -m, --meta     Dump file meta content.";
  LOG(INFO) << " -b, --blk      Export blk device content.";
  LOG(INFO) << " -h, --help     Print help info.";

  LOG(INFO) << "Examples:";
  LOG(INFO) << " help    : ./block_fs_tool -h";
  LOG(INFO) << " format  : ./block_fs_tool -d /dev/vdb -f";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p check";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p negot";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p super";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p uuid";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p dirs";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p files";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb -p all";
  LOG(INFO) << " meta    : ./block_fs_tool -d /dev/vdb -m file_name";
  LOG(INFO) << " dump    : ./block_fs_tool -d /dev/vdb --dump xxx";
  LOG(INFO) << " blk     : ./block_fs_tool -d /dev/vdb -b offset:size";
}


std::vector<std::string> StringSplit(const std::string &s,
                                     const std::string &delim = " ") {
  std::vector<std::string> elems;
  size_t pos = 0;
  size_t len = s.length();
  size_t delim_len = delim.length();
  if (delim_len == 0) return elems;
  while (pos < len) {
    int find_pos = s.find(delim, pos);
    if (find_pos < 0) {
      elems.push_back(s.substr(pos, len - pos));
      break;
    }
    elems.push_back(s.substr(pos, find_pos - pos));
    pos = find_pos + delim_len;
  }
  return elems;
}

void BlockFsTool::PrintFileMetadata(const std::string &file_name) {
  FileSystem::Instance()->DumpFileMeta(file_name);
}

bool BlockFsTool::ExportBlkDeviceContents() {
  std::vector<std::string> info = StringSplit(blk_info_, ":");
  int64_t export_offset = std::atol(info[0].c_str());
  int64_t export_size = std::atol(info[1].c_str());

  static const int64_t kExportBufferSize = 16 * 1024;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(kExportBufferSize);

  std::string dst = "blk_" + blk_info_;
  int32_t dst_fd = ::open(dst.c_str(), O_WRONLY | O_CREAT, 0777);
  if (dst_fd < 0) {
    LOG(ERROR) << "failed to open file:" << dst << ", error:" << errno;
    return false;
  }
  if (::fallocate(dst_fd, 0, 0, export_size) < 0) {
    LOG(ERROR) << "failed to fallocate file: " << dst << ", error:" << errno;
    ::close(dst_fd);
    return false;
  }

  int64_t ret;
  int64_t read_offset = export_offset;
  while (export_size > 0) {
    ::memset(buffer->data(), 0, kExportBufferSize);
    if (export_size >= kExportBufferSize) {
      ret = FileSystem::Instance()->dev()->PreadCache(
          buffer->data(), kExportBufferSize, read_offset);
      if (ret != kExportBufferSize) {
        ::close(dst_fd);
        return false;
      }
      ret = ::pwrite(dst_fd, buffer->data(), kExportBufferSize, read_offset);
      if (ret != kExportBufferSize) {
        ::close(dst_fd);
        return false;
      }
      export_size -= kExportBufferSize;
      read_offset += kExportBufferSize;
    } else {
      ret = FileSystem::Instance()->dev()->PreadCache(buffer->data(),
                                                     export_size, read_offset);
      if (ret != export_size) {
        ::close(dst_fd);
        return false;
      }
      ret = ::pwrite(dst_fd, buffer->data(), export_size, read_offset);
      if (ret != export_size) {
        ::close(dst_fd);
        return false;
      }
      if (::fsync(dst_fd) < 0) {
        LOG(ERROR) << "failed to fsync file: " << dst << ", error:" << errno;
        ::close(dst_fd);
        return false;
      }
      export_size = 0;
    }
  }
  ::close(dst_fd);
  LOG(INFO) << "Export: " << blk_info_ << " to " << dst << " success";
  return true;
}

bool BlockFsTool::DoBlockFsTool() {
  bool is_success = false;
  auto start = std::chrono::high_resolution_clock::now();
  if (tool_type_ == BLOCKFS_FORMAT) {
    is_success = FileSystem::Instance()->Format(dev_name_);
    if (!is_success) {
      LOG(INFO) << "format block fs failed";
    } else {
      LOG(INFO) << "format block fs success";
    }
  } else {
    is_success = FileSystem::Instance()->Check(dev_name_);
    if (!is_success) [[unlikely]] {
      LOG(ERROR) << "check load BlockFS failed";
      return false;
    }
    switch (tool_type_) {
      case BLOCKFS_DUMP:
        if (dump_arg_.find("check") != std::string::npos) {
          LOG(INFO) << "check load block fs success";
        } else if (dump_arg_.find("super") != std::string::npos) {
          FileSystem::Instance()->super()->Dump();
        } else if (dump_arg_.find("uuid") != std::string::npos) {
          LOG(INFO) << "dump uuid: " << FileSystem::Instance()->super()->uuid();
        } else if (dump_arg_.find("files") != std::string::npos) {
          FileSystem::Instance()->file_handle()->Dump();
        } else {
          FileSystem::Instance()->super()->Dump();
          FileSystem::Instance()->file_handle()->Dump();
        }
        break;
      case BLOCKFS_BLK:
        is_success = ExportBlkDeviceContents();
        break;
      case BLOCKFS_FILE_META:
        PrintFileMetadata(file_name_);
        break;
      default:
        is_success = false;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::ratio<1, 1>> duration_s(end - start);
  LOG(INFO) << "total cost time: " << duration_s.count() << " seconds";
  return is_success;
}

int main(int argc, char *argv[]) {
  BlockFsTool tool = BlockFsTool();
  tool.ParseOption(argc, argv);
  return tool.DoBlockFsTool() ? 0 : -1;
}