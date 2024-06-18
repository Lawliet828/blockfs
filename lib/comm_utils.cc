#include "comm_utils.h"

#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace udisk::blockfs {

// new_dirname = ::dirname(full_path.c_str());
// new_filename = ::basename(full_path.c_str());
std::string GetFileName(const std::string &path) {
  char ch = '/';
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

// 返回目录名, 末尾需要带'/'，如果是根目录则返回'/'
std::string GetDirName(const std::string &path) {
  std::string tmp = path;
  if (tmp == "/") {
    return tmp;
  }
  if (tmp.back() == '/') {
    tmp.pop_back();
  }
  std::string parent_path = fs::path(tmp).parent_path().string();
  if (parent_path == "/") {
    return parent_path;
  } else {
    return parent_path + "/";
  }
}

}