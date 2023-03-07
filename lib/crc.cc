#include "crc.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

CRCFunction g_Crc32;

uint32_t Crc32(uint8_t *chunk, size_t len) {
  if (g_Crc32)
    return g_Crc32(chunk, len);
  else
    return ChecksumSW(chunk, len);
}

}  // namespace blockfs
}  // namespace udisk