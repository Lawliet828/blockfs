// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_META_CONSTS_H
#define LIB_META_CONSTS_H

#include <stdint.h>

namespace udisk {
namespace blockfs {

const uint64_t K = 1024;
const uint64_t M = (1024 * K);
const uint64_t G = (1024 * M);
const uint64_t T = (1024 * G);

#define KB *(1 << 10)
#define MB *(1 << 20)
#define GB *(1U << 30)

const uint32_t kBlockFsVersion = 0x1;
const uint32_t kBlockFsMagic = 0xA5201314;

const uint64_t kBlockFsMaxUuidSize = 64;
const uint64_t kUXDBMountPrefixMaxLen = 256;

const uint64_t kBlockFsMaxUxdbPrefixDirLen = (2 * K);

const uint64_t kBlockFsPageSize = (4 * K);
const uint64_t kNegotiationSize = kBlockFsPageSize;
const uint64_t kSuperBlockSize = kBlockFsPageSize;
const uint64_t kNegotiationOffset = 0;
const uint64_t kSuperBlockOffset = (kNegotiationOffset + kNegotiationSize);
const uint64_t kDirMetaOffset = (kSuperBlockOffset + kSuperBlockSize);

const uint64_t kBlockFsMaxFileNum = 100000;
const uint64_t kBlockFsMaxDirNum = kBlockFsMaxFileNum;
const uint64_t kBlockFsMaxUDiskSize = (128 * T);
const uint64_t kBlockFsExtendSize = (10 * G);
const uint64_t kBlockFsBlockSize = (16 * M);
const uint64_t kBlockFsMaxDirNameLen = 768;
const uint64_t kBlockFsMaxFileNameLen = 64;
const uint64_t kBlockFsDirMetaSize = (1 * K);
const uint64_t kBlockFsFileMetaSize = 256;
const uint64_t kBlockFsFileBlockCapacity = 1000;
const uint64_t kBlockFsFileBlockMetaSize = kBlockFsPageSize;
const uint64_t kBlockFsFileMetaIndexSize = kBlockFsPageSize;
const uint64_t kBlockFsJournalSize = kBlockFsPageSize;
const uint64_t kBlockFsJournalNum = (4 * K);
}  // namespace blockfs
}  // namespace udisk
#endif
