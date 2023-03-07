// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_CRC_H
#define LIB_CRC_H

#include <stdint.h>

#include <functional>

#define HAVE_CRC32_HARDWARE 1

namespace udisk {
namespace blockfs {

uint32_t Crc32(uint8_t *chunk, size_t len);

typedef std::function<uint32_t(uint8_t *, size_t)> CRCFunction;

// from mongodb
void ChecksumInit(void);

uint32_t ChecksumSW(const uint8_t *chunk, size_t len)
    __attribute__((visibility("default")));

}  // namespace blockfs
}  // namespace udisk
#endif
