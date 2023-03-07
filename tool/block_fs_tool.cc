// Copyright (c) 2020 UCloud All rights reserved.
#include <fcntl.h>
#include <getopt.h>
#include <sys/cdefs.h>

#include <chrono>
#include <iomanip>

#include "crc.h"
#include "file_store_udisk.h"
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
    BLOCKFS_DF,
    BLOCKFS_SET_SIZE,
    BLOCKFS_GET_SIZE,
  };
  ToolType tool_type_ = BLOCKFS_NONE;
  std::string dump_arg_;
  std::string dev_name_;
  std::string blk_info_;  // offset and size
  std::string file_name_;
  uint64_t available_size_; // 可用空间大小

 private:
  void HelpInfo();

 public:
  BlockFsTool();
  ~BlockFsTool();
  void ParseOption(int argc, char **argv);
  bool DoBlockFsTool();

 public:
  void PrintFileMetadata(const std::string &file_name);
  bool ExportBlkDeviceContents();
};

BlockFsTool::BlockFsTool() {}

BlockFsTool::~BlockFsTool() {}

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
        {"df", no_argument, nullptr, 'D'},
        {"set", required_argument, nullptr, 'S'},
        {"get", no_argument, nullptr, 'G'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "m:b:d:p:fDS:Gvh?", longOpts, &optIndex);
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
      case 'D': {
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_DF;
      } break;
      case 'S': {
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
        tool_type_ = BLOCKFS_SET_SIZE;
        int64_t size = atoll(optarg);
        if (size <= 0) {
          LOG(ERROR) << "input size is illegal";
          exit(1);
        }
        available_size_ = static_cast<uint64_t>(size);
        LOG(INFO) << "available_size: " << available_size_;
      } break;
      case 'G': {
        if (tool_type_ != BLOCKFS_NONE) {
          LOG(ERROR) << "other option already exist, Please check";
          HelpInfo();
          exit(1);
        }
        tool_type_ = BLOCKFS_GET_SIZE;
      } break;
      case 'v':
      case 'h':
      case '?':
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
  LOG(INFO) << " -D, --df       Check disk usage.";
  LOG(INFO) << " -S, --set      Set available size.";
  LOG(INFO) << " -G, --get      Get available size.";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";

  LOG(INFO) << "Examples:";
  LOG(INFO) << " help    : ./block_fs_tool -h";
  LOG(INFO) << " version : ./block_fs_tool -v";
  LOG(INFO) << " format  : ./block_fs_tool -d /dev/vdb -f";
  LOG(INFO) << " format  : ./block_fs_tool -d /dev/vdb --format";
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
  LOG(INFO) << " set     : ./block_fs_tool -d /dev/vdb -S 21474836480";
  LOG(INFO) << " get     : ./block_fs_tool -d /dev/vdb -G";
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
  FileStore::Instance()->DumpFileMeta(file_name);
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
      ret = FileStore::Instance()->dev()->PreadCache(
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
      ret = FileStore::Instance()->dev()->PreadCache(buffer->data(),
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
    is_success = FileStore::Instance()->Format(dev_name_);
    if (!is_success) {
      LOG(INFO) << "format block fs failed";
    } else {
      LOG(INFO) << "format block fs success";
    }
  } else if (BLOCKFS_GET_SIZE == tool_type_) {
    // 该命令使用人员希望只返回值, 不要debug信息
    is_success = FileStore::Instance()->Check(dev_name_, "ERROR");
    if (unlikely(!is_success)) {
      LOG(ERROR) << "check load BlockFS failed";
      return false;
    }
    std::cout << FileStore::Instance()->super()->get_available_udisk_size() << std::endl;
  } else {
    is_success = FileStore::Instance()->Check(dev_name_);
    if (unlikely(!is_success)) {
      LOG(ERROR) << "check load BlockFS failed";
      return false;
    }
    switch (tool_type_) {
      case BLOCKFS_DUMP:
        if (dump_arg_.find("check") != std::string::npos) {
          LOG(INFO) << "check load block fs success";
        } else if (dump_arg_.find("negot") != std::string::npos) {
          FileStore::Instance()->negot()->Dump();
        } else if (dump_arg_.find("super") != std::string::npos) {
          FileStore::Instance()->super()->Dump();
        } else if (dump_arg_.find("uuid") != std::string::npos) {
          LOG(INFO) << "dump uuid: " << FileStore::Instance()->super()->uuid();
        } else if (dump_arg_.find("dirs") != std::string::npos) {
          FileStore::Instance()->dir_handle()->Dump();
        } else if (dump_arg_.find("files") != std::string::npos) {
          FileStore::Instance()->file_handle()->Dump();
        } else {
          FileStore::Instance()->negot()->Dump();
          FileStore::Instance()->super()->Dump();
          FileStore::Instance()->dir_handle()->Dump();
          FileStore::Instance()->file_handle()->Dump();
          FileStore::Instance()->journal_handle()->Dump();
        }
        break;
      case BLOCKFS_BLK:
        is_success = ExportBlkDeviceContents();
        break;
      case BLOCKFS_FILE_META:
        PrintFileMetadata(file_name_);
        break;
      case BLOCKFS_DF: {
        uint64_t dev_size = FileStore::Instance()->dev()->dev_size();
        uint64_t meta_size = FileStore::Instance()->super()->meta_size();
        uint64_t curr_block_num = FileStore::Instance()->super()->curr_block_num();
        uint64_t free_block_num = FileStore::Instance()->block_handle()->GetFreeBlockNum();
        uint64_t used_block_num = curr_block_num - free_block_num;
        uint64_t used_size = meta_size + 16 * M * used_block_num;
        uint64_t avail_size = 16 * M * free_block_num;
        assert(used_size + avail_size == dev_size);
        int32_t use_percent = used_size * 100 / dev_size;
        LOG(INFO) << "\n"
                  << "Size\tUsed\tAvail\tUse%\n"
                  << dev_size << "\t" << used_size << "\t"
                  << avail_size << "\t" << use_percent << "%";
        } break;
      case BLOCKFS_SET_SIZE:
        is_success = FileStore::Instance()->super()->set_available_udisk_size(available_size_);
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