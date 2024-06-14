#include "comm_utils.h"

#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>

namespace udisk::blockfs {

// https://blog.csdn.net/butterfly5211314/article/details/84575883
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

std::string GetDirName(const std::string &path) {
  return std::filesystem::path(path).parent_path().string();
}

std::string GetParentDirName(const std::string &path) {
  std::string parent = path;
  if (parent == "/") {
    return parent;
  }
  if (parent[parent.size() - 1] == '/') {
    parent.erase(parent.size() - 1);
  }
  return GetDirName(parent);
}

}