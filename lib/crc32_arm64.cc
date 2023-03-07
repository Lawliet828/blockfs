
#include "comm_utils.h"
#include "crc.h"
#include "logging.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

extern CRCFunction g_Crc32;

#if defined(_M_ARM64) && defined(HAVE_CRC32_HARDWARE)

#include <asm/hwcap.h>
#include <sys/auxv.h>

#ifndef __GNUC__
#define __asm__ asm
#endif

#define CRC32CX(crc, value) \
  __asm__("crc32cx %w[c], %w[c], %x[v]" : [ c ] "+r"(*&crc) : [ v ] "r"(+value))
#define CRC32CW(crc, value) \
  __asm__("crc32cw %w[c], %w[c], %w[v]" : [ c ] "+r"(*&crc) : [ v ] "r"(+value))
#define CRC32CH(crc, value) \
  __asm__("crc32ch %w[c], %w[c], %w[v]" : [ c ] "+r"(*&crc) : [ v ] "r"(+value))
#define CRC32CB(crc, value) \
  __asm__("crc32cb %w[c], %w[c], %w[v]" : [ c ] "+r"(*&crc) : [ v ] "r"(+value))

/*
 * ChecksumHW --
 *	Return a checksum for a chunk of memory, computed in hardware
 *	using 8 byte steps.
 */
static uint32_t ChecksumHW(const uint8_t *chunk, size_t len) {
  uint32_t crc;
  size_t nqwords;
  const uint8_t *p;
  const uint64_t *p64;

  crc = 0xffffffff;

  /* Checksum one byte at a time to the first 4B boundary. */
  for (p = chunk; ((uintptr_t)p & (sizeof(uint32_t) - 1)) != 0 && len > 0;
       ++p, --len) {
    CRC32CB(crc, *p);
  }

  p64 = (const uint64_t *)p;
  /* Checksum in 8B chunks. */
  for (nqwords = len / sizeof(uint64_t); nqwords; nqwords--) {
    CRC32CX(crc, *p64);
    p64++;
  }

  /* Checksum trailing bytes one byte at a time. */
  p = (const uint8_t *)p64;
  for (len &= 0x7; len > 0; ++p, len--) {
    CRC32CB(crc, *p);
  }

  return (~crc);
}
#endif

/*
 * ChecksumInit --
 *	WiredTiger: detect CRC hardware and set the checksum function.
 */
__attribute_cold__ __attribute_noinline__ void ARM64ChecksumInit(void) {
#if defined(_M_ARM64) && defined(HAVE_CRC32_HARDWARE)
  unsigned long caps = ::getauxval(AT_HWCAP);

  if (caps & HWCAP_CRC32)
    g_Crc32 = ChecksumHW;
  else
    g_Crc32 = ChecksumSW;

#else
  g_Crc32 = ChecksumSW;
#endif
}

}  // namespace blockfs
}  // namespace udisk