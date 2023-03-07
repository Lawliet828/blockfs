// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_ALIGNED_BUFFER_H_
#define LIB_ALIGNED_BUFFER_H_

#include "comm_utils.h"
#include "meta_defines.h"

// https://github.com/OpenMPDK/KVRocks/tree/master/kvdb/util
namespace udisk {
namespace blockfs {

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <new>
#include <string>

class AlignBuffer {
 public:
  static const bool kFillZero = true;

 public:
  AlignBuffer(uint64_t sz, uint32_t alignment = 512, bool fill_zero = kFillZero)
      : size_(sz), alignment_(alignment), data_(nullptr) {
    // the alignment must be a power of 2 and a multiple of sizeof(void *)
    // see man(3) posix_memalign
    if ((alignment_ & (alignment_ - 1)) != 0 ||
        alignment_ % sizeof(void *) != 0) {
      throw std::bad_alloc();
    }
    int ret =
        ::posix_memalign(reinterpret_cast<void **>(&data_), alignment_, size_);
    if (ret != 0 || data_ == nullptr) {
      throw std::bad_alloc();
    }
    if (fill_zero) zero();
  }
  virtual ~AlignBuffer() { ::free(data_); }

 public:
  void zero() {
    if (data_) {
      ::memset(data_, 0, size_);
    }
  }
  virtual char *data() { return data_; }
  virtual uint32_t size() { return size_; }

 private:
  uint64_t size_;
  uint32_t alignment_;
  char *data_;
};

typedef std::shared_ptr<AlignBuffer> AlignBufferPtr;

struct ReadDataBuf {
  AlignBufferPtr buf;
  uint32_t start;
  uint32_t size;
};

typedef std::shared_ptr<ReadDataBuf> ReadDataBufPtr;
typedef std::vector<ReadDataBufPtr> ReadDataBufPtrVec;

}  // namespace blockfs
}  // namespace udisk
#endif