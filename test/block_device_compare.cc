#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>

/* Display a buffer into a HEXA formated output */
void DumpBuffer(const char *buff, size_t count, FILE *fp) {
  size_t i, j, c;
  bool printnext = true;

  if (count % 16)
    c = count + (16 - count % 16);
  else
    c = count;

  for (i = 0; i < c; i++) {
    if (printnext) {
      printnext = false;
      fprintf(fp, "%.4zu ", i & 0xffff);
    }
    if (i < count)
      fprintf(fp, "%3.2x", buff[i] & 0xff);
    else
      fprintf(fp, "   ");

    if (!((i + 1) % 8)) {
      if ((i + 1) % 16) {
        fprintf(fp, " -");
      } else {
        fprintf(fp, "   ");
        for (j = i - 15; j <= i; j++) {
          if (j < count) {
            if ((buff[j] & 0xff) >= 0x20 && (buff[j] & 0xff) <= 0x7e) {
              fprintf(fp, "%c", buff[j] & 0xff);
            } else {
              fprintf(fp, ".");
            }
          } else {
            fprintf(fp, " ");
          }
        }
        fprintf(fp, "\n");
        printnext = true;
      }
    }
  }
}

void PrintBuffer(void *pBuff, unsigned int nLen) {
  if (NULL == pBuff || 0 == nLen) {
    return;
  }

  const int nBytePerLine = 16;
  unsigned char *p = (unsigned char *)pBuff;
  char szHex[3 * nBytePerLine + 1] = {0};

  printf("-----------------begin-------------------\n");
  for (unsigned int i = 0; i < nLen; ++i) {
    int idx = 3 * (i % nBytePerLine);
    if (0 == idx) {
      memset(szHex, 0, sizeof(szHex));
    }
#ifdef WIN32
    sprintf_s(&szHex[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#else
    snprintf(&szHex[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#endif

    // 以16个字节为一行，进行打印
    if (0 == ((i + 1) % nBytePerLine)) {
      printf("%s\n", szHex);
    }
  }

  // 打印最后一行未满16个字节的内容
  if (0 != (nLen % nBytePerLine)) {
    printf("%s\n", szHex);
  }

  printf("------------------end-------------------\n");
}

int main(int argc, char *argv[]) {
  int flags = O_RDONLY | O_LARGEFILE | O_DIRECT | O_CLOEXEC;

  // 不需要额外的access检查,如果不存在的话会报错
  int fd0 = ::open("/dev/nbd0", flags);
  if (fd0 < 0) {
    std::cout << "Failed to open block device: "
              << "/dev/nbd0" << std::endl;
    return -1;
  }
  int fd1 = ::open("/dev/nbd1", flags);
  if (fd1 < 0) {
    std::cout << "Failed to open block device: "
              << "/dev/nbd1" << std::endl;
    return -1;
  }

  void *raw_buffer = NULL;
  // 4M的分片
  int ret = ::posix_memalign(&raw_buffer, 512, 4 * 1024 * 1024);
  if (ret < 0 || !raw_buffer) {
    return -1;
  }
  char *buffer0 = static_cast<char *>(raw_buffer);

  ret = ::posix_memalign(&raw_buffer, 512, 4 * 1024 * 1024);
  if (ret < 0 || !raw_buffer) {
    return -1;
  }
  char *buffer1 = static_cast<char *>(raw_buffer);

  int64_t offset = 0;
  while (true) {
    ::memset(buffer0, 0, 4 * 1024 * 1024);
    if (::pread(fd0, buffer0, 4 * 1024 * 1024, offset * 4 * 1024 * 1024) < 0) {
      std::cout << "Read buffer0 error";
    }
    ::memset(buffer1, 0, 4 * 1024 * 1024);
    if (::pread(fd1, buffer1, 4 * 1024 * 1024, offset * 4 * 1024 * 1024) < 0) {
      std::cout << "Read buffer1 error";
    }
    if (::memcmp(buffer0, buffer1, 4 * 1024 * 1024) != 0) {
      std::cout << "Offset: " << offset << " not compare" << std::endl;
      std::cout << "Buffer0: " << buffer0 << std::endl;

      PrintBuffer(buffer0, 4 * 1024 * 1024);
      std::cout << "Buffer1: " << buffer1 << std::endl << std::endl;
      PrintBuffer(buffer1, 4 * 1024 * 1024);
      // FILE *fp = ::fopen("nbd_diff.log", "rb");
      // fprintf(fp, "   ");
      // // fwrite(fp, "Buffer-0");
      // fprintf(fp, "Buffer-0");
      // DumpBuffer(buffer0, 4 * 1024 * 1024, fp);
      // fprintf(fp, "\n\nBuffer-1");
      // DumpBuffer(buffer0, 4 * 1024 * 1024, fp);
      return -1;
    } else {
      std::cout << "Offset: " << offset << " is ok" << std::endl;
      offset += 1;
    }
  }
  return 0;
}