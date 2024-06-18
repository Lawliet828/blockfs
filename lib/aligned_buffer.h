#ifndef LIB_ALIGNED_BUFFER_H_
#define LIB_ALIGNED_BUFFER_H_

#include "meta_defines.h"

namespace udisk::blockfs {

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <new>

class AlignBuffer {
 public:
  static const bool kFillZero = true;

 public:
  AlignBuffer(size_t size, uint32_t alignment = 512, bool fill_zero = kFillZero)
      : size_(size), alignment_(alignment), data_(nullptr) {
    int ret =
        ::posix_memalign(reinterpret_cast<void **>(&data_), alignment_, size_);
    if (ret != 0 || data_ == nullptr) {
      throw std::bad_alloc();
    }
    if (fill_zero) {
      ::memset(data_, 0, size_);
    }
  }
  virtual ~AlignBuffer() { ::free(data_); }

  char *data() { return data_; }
  size_t size() { return size_; }

 private:
  size_t size_;
  uint32_t alignment_;
  char *data_;
};

typedef std::shared_ptr<AlignBuffer> AlignBufferPtr;

}
#endif