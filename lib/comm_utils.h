
#ifndef LIB_COMM_UTILS_H_
#define LIB_COMM_UTILS_H_

#include <string>
namespace udisk::blockfs {

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/* Meta 区域大小保证4K对齐 */
#define ALIGN_UP(size, align) (((size) + (align - 1)) / (align))
// #define ALIGN_UP(size, align) (((size) + (align - 1)) & (~(align - 1)))
#define ROUND_UP(size, align) ((((size) + (align - 1)) / (align)) * (align))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

std::string GetFileName(const std::string &path);
std::string GetDirName(const std::string &path);
std::string GetParentDirName(const std::string &path);

bool SetCoreDump(bool on = true);

bool RunAsAdmin();

}
#endif