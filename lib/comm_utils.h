
// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_COMM_UTILS_H_
#define LIB_COMM_UTILS_H_

#include <string>
namespace udisk {
namespace blockfs {

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/* Meta 区域大小保证4K对齐 */
#define ALIGN_UP(size, align) (((size) + (align - 1)) / (align))
// #define ALIGN_UP(size, align) (((size) + (align - 1)) & (~(align - 1)))
#define ROUND_UP(size, align) ((((size) + (align - 1)) / (align)) * (align))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define BLOCK_FS_INITIALIZER(func, level)                     \
  static void func(void) __attribute__((constructor(level))); \
  static void func(void)

#ifndef __attribute_cold__
#if __has_attribute(cold) || __GNUC_PREREQ(4, 3)
#define __attribute_cold__ __attribute__((__cold__))
#else
#define __attribute_cold__
#endif
#endif

#ifndef __attribute_hot__
#if __has_attribute(hot) || __GNUC_PREREQ(4, 3)
#define __attribute_hot__ __attribute__((__hot__))
#else
#define __attribute_hot__
#endif
#endif

std::string GetFileName(const std::string &path);
std::string GetDirName(const std::string &path);
std::string GetParentDirName(const std::string &path);

bool SetCoreDump(bool on = true);

bool RunAsAdmin();

}  // namespace blockfs
}  // namespace udisk
#endif