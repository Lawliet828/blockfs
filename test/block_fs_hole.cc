// Copyright (c) 2020 UCloud All rights reserved.
#include <fcntl.h>
#include <getopt.h>

#include <chrono>
#include <deque>
#include <iostream>

#include "file_system.h"
#include "logging.h"
using namespace udisk::blockfs;

// https://www.dazhuanlan.com/2019/12/16/5df68dd4e8d27/

// 用fallocate进行"文件预留"或"文件打洞"
// https://www.it610.com/article/1282139573257781248.htm

// 慢慢欣赏linux 文件黑洞和文件空洞
// https://blog.csdn.net/shipinsky/article/details/86635160?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.channel_param
int main(int argc, char *argv[]) {
  int64_t ret = -1;
  int32_t fd = -1;
  std::string buffer(4096, '1');
  mode_t f_attrib = S_IRUSR | S_IWUSR;
#if 1
  fd = ::open("/home/ubuntu/hole/file.hole", O_RDWR | O_CREAT | O_TRUNC,
              f_attrib);
  if (fd < 0) {
    LOG(ERROR) << "Failed to create file.hole";
    return -1;
  }
  struct stat st;
  while (true) {
    ret = ::write(fd, buffer.data(), buffer.size());
    ::memset(&st, 0, sizeof(st));
    fstat(fd, &st);
    LOG(INFO) << "Write ret: " << ret << " file size: " << st.st_size;
    if (ret < 0) {
      break;
    }
  }
  ::fdatasync(fd);

  ::close(fd);

  ret = ::unlink("/home/ubuntu/hole/file.hole");
  if (ret < 0) {
    LOG(ERROR) << "Failed to unlink file.hole";
    return -1;
  }

  ret = ::stat("/home/ubuntu/hole/file.hole", &st);
  if (ret < 0) {
    if (errno != ENOENT) {
      LOG(INFO) << "unlink file failed, file exist";
      return -1;
    }
  }
#endif
  void *raw_buffer = nullptr;
  ret = ::posix_memalign(&raw_buffer, 512, 256 * 1024);
  if (ret < 0 || !raw_buffer) {
    return -1;
  }
  char *o_buffer = static_cast<char *>(raw_buffer);

#if 1
  fd = ::open("/home/ubuntu/hole/file.hole2", O_RDWR | O_CREAT, 0);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open file.hole2, errno: " << errno;
    return -1;
  }

  ::memset(o_buffer, 0, 256 * 1024);
  ret = ::pread(fd, o_buffer, 8192, 0);
  if (ret < 0) {
    LOG(ERROR) << "Failed to read file.hole2";
    return -1;
  }
  std::cout << "read ret: " << ret << std::endl;
  std::cout << "read empty file, buffer:" << o_buffer << std::endl;

  // 4096 + 1024 + 2
  std::string buffer2(5122, '2');
  ret = ::write(fd, buffer2.data(), buffer2.size());
  if (ret < 0) {
    LOG(ERROR) << "Failed to write file.hole2";
    return -1;
  }
  ::fdatasync(fd);

  ret = ::ftruncate(fd, 8192);
  if (ret < 0) {
    LOG(ERROR) << "Failed to ftruncate file.hole2, errno: " << errno;
    return -1;
  }

  ::memset(o_buffer, 0, 256 * 1024);
  ret = ::pread(fd, o_buffer, 8192, 0);
  if (ret < 0) {
    LOG(ERROR) << "Failed to read file.hole2";
    return -1;
  }
  std::cout << "read ret: " << ret << std::endl;
  std::cout << "read file, buffer:" << o_buffer << std::endl;

  ::memset(o_buffer, 0, 256 * 1024);
  ret = ::pread(fd, o_buffer, 3072, 5120);
  if (ret < 0) {
    LOG(ERROR) << "Failed to read file.hole2";
    return -1;
  }
  std::cout << "read ret: " << ret << std::endl;
  std::cout << "read file, buffer:" << o_buffer << std::endl;

  ::fdatasync(fd);
  ::close(fd);

  ret = ::unlink("/home/ubuntu/hole/file.hole2");
  if (ret < 0) {
    LOG(ERROR) << "Failed to unlink file.hole";
    return -1;
  }
  return 0;
#endif

  Device *dev = new Device();
  if (!dev->Open("/dev/vdc1")) {
    return -1;
  }

  uint64_t offset = 0;
  while (offset < dev->dev_size()) {
    ::memset(o_buffer, 0, 256 * 1024);
    ret = dev->PreadDirect(o_buffer, 256 * 1024, offset);
    if (ret < 0) {
      LOG(ERROR) << "Failed to read buffer, offset: " << offset;
      break;
    }
    if (*o_buffer != 0) {
      std::cout << "offset: " << offset << " buffer not zero" << std::endl;
      // std::cout << "buffer:" << o_buffer << std::endl << std::endl;
    }
    offset += 256 * 1024;
  }

  // 用ll查看的文件大小
  // file.hole: 40k
  // lseek.hole: 8k

  // du -sh *查看
  // file.hole: 28k 少的12k其实是fallocate归还给文件系统了
  // lseek.hole: 4k 少的那4k是文件空洞，文件系统并没有分磁盘块给它

  // du和ls查看文件大小的区别
  // 通常情况下, ls显示的文件大小比du显示的磁盘占用空间小,
  // 比如文件系统的block是4K，一个13K的文件占用的空间是 13k/4k = 3.25 个block
  // 一个block只能被一个文件占用，因此实际占用空间就是4个block，就是16K。

  // 如果一个文件有比较大的黑洞，那么会出现文件大小比磁盘空间占用大的情况

  // https://www.zhihu.com/question/407305048
  // hexdump lseek.hole
  // od -c hole.file
  if (::write(fd, buffer.data(), 40960) < 0) {
    LOG(ERROR) << "Failed to write data";
    return -1;
  }
  ::fdatasync(fd);

  //在4k偏移的位置上打一个12k大小的洞
  if (::fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 4096,
                  4096 * 3) < 0) {
    LOG(ERROR) << "Failed to fallocate file";
    return -1;
  }

  if (::lseek(fd, 4098, SEEK_SET) < 0) {
    LOG(ERROR) << "Failed to lseek file";
    return -1;
  }

  char c = 'q';
  if (::read(fd, &c, 1) < 0) {
    LOG(ERROR) << "Failed to read file";
    return -1;
  }
  LOG(INFO) << "Read char: " << c;
  if (::read(fd, &c, 1) < 0) {
    LOG(ERROR) << "Failed to read file";
    return -1;
  }
  // 验证打洞范围的文件读的是0
  LOG(INFO) << "Read char: " << c;

  fd = open("/home/ubuntu/hole/lseek.hole", O_RDWR | O_CREAT | O_TRUNC,
            f_attrib);
  if (fd < 0) {
    LOG(ERROR) << "Failed to fallocate file";
    return -1;
  }
  // 创建4k的空洞
  if (::lseek(fd, 4096, SEEK_SET) < 0) {
    LOG(ERROR) << "Failed to lseek file";
    return -1;
  }
  // 从4k偏移处写4k的内容到文件
  if (::write(fd, buffer.data(), 4096) < 0) {
    LOG(ERROR) << "Failed to write file";
    return -1;
  }

  fd = open("/home/ubuntu/hole/preallocate.hole", O_RDWR | O_CREAT | O_TRUNC,
            f_attrib);
  if (fd < 0) {
    LOG(ERROR) << "Failed to fallocate file";
    return -1;
  }

  // 我的ubuntu ext4总是失败, 返回 Operation not supported
  if (::fallocate(fd, 0, 0, 4096) < 0) {
    LOG(ERROR) << "Failed to preallocate file, error:" << strerror(errno);
    return -1;
  }
  return 0;
}