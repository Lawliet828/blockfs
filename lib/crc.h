#pragma once

#include <stdint.h>
#include <zlib.h>

inline uint32_t Crc32(uint8_t *chunk, size_t len) {
  uint32_t crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, chunk, len);
  return crc;
}

/*
在 zlib 库中，crc32 函数的设计允许你在多个数据块上累积计算 CRC。第一次调用 crc32 函数是为了初始化 CRC 值，第二次调用则是为了计算给定数据块的 CRC。

在第一次调用中，crc32 函数的第一个参数是 0L，表示初始化 CRC 值，第二个参数是 Z_NULL，表示没有数据块, 第三个参数是数据块的长度
在第二次调用中，第一个参数是前一次调用 crc32 返回的 CRC 值，第二个参数是要计算 CRC 的数据块。

如果你有多个数据块需要计算 CRC，你可以继续调用 crc32 函数，每次都传入前一次调用返回的 CRC 值和新的数据块。
*/
