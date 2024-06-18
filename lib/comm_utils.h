#ifndef LIB_COMM_UTILS_H_
#define LIB_COMM_UTILS_H_

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace udisk::blockfs {

/* Meta 区域大小保证4K对齐 */
#define ALIGN_UP(size, align) (((size) + (align - 1)) / (align))
// #define ALIGN_UP(size, align) (((size) + (align - 1)) & (~(align - 1)))
#define ROUND_UP(size, align) ((((size) + (align - 1)) / (align)) * (align))

inline std::string GetFileName(const std::string &path) {
  char ch = '/';
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

// 返回目录名, 末尾需要带'/'，如果是根目录则返回'/'
inline std::string GetDirName(const std::string &path) {
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
#endif