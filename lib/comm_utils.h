
#ifndef LIB_COMM_UTILS_H_
#define LIB_COMM_UTILS_H_

#include <string>
namespace udisk::blockfs {

#define unlikely(x) __builtin_expect((x), 0)

/* Meta 区域大小保证4K对齐 */
#define ALIGN_UP(size, align) (((size) + (align - 1)) / (align))
// #define ALIGN_UP(size, align) (((size) + (align - 1)) & (~(align - 1)))
#define ROUND_UP(size, align) ((((size) + (align - 1)) / (align)) * (align))

std::string GetFileName(const std::string &path);
std::string GetDirName(const std::string &path);
std::string GetParentDirName(const std::string &path);

bool RunAsAdmin();

}
#endif