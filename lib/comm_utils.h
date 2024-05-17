
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

// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertable to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template <typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

class Copyable {
 protected:
  Copyable() = default;
  ~Copyable() = default;
};

std::string GetFileName(const std::string &path);
std::string GetDirName(const std::string &path);
std::string GetParentDirName(const std::string &path);

bool SetCoreDump(bool on = true);

void DebugHexBuffer(void *buffer, uint32_t len);

bool RunAsAdmin();

}  // namespace blockfs
}  // namespace udisk
#endif