#include "block_device.h"

#include <fcntl.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/fs.h>

#include "logging.h"

namespace udisk::blockfs {

static const std::string kBlockDeviceSda = "/dev/sda";
static const std::string kBlockDeviceVda = "/dev/vda";
static const std::string kBlockDeviceCurrent = "/dev/.";
static const std::string kBlockDeviceParent = "/dev/..";

static const int kBlkOpenWithDirect =
    O_RDWR | O_LARGEFILE | O_DIRECT | O_CLOEXEC;
static const int kBlkOpenWithoutDirect = O_RDWR | O_LARGEFILE | O_CLOEXEC;

inline void incr(int64_t /* n */) {}
inline void incr(int64_t n, off_t &offset) { offset += off_t(n); }

// https://github.com/facebook/folly/blob/main/folly/detail/FileUtilDetail.h#L58
// Wrap call to read/pread/write/pwrite(fd, buf, count, offset?) to retry on
// incomplete reads / writes.  The variadic argument magic is there to support
// an additional argument (offset) for pread / pwrite; see the incr() functions
// above which do nothing if the offset is not present and increment it if it
// is.
template <class F, class... Offset>
ssize_t wrapFull(F f, int fd, void* buf, size_t count, Offset... offset) {
  char* b = static_cast<char*>(buf);
  ssize_t totalBytes = 0;
  ssize_t r;
  do {
    r = f(fd, b, count, offset...);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      return r;
    }

    totalBytes += r;
    b += r;
    count -= r;
    incr(r, offset...);
  } while (r != 0 && count); // 0 means EOF

  return totalBytes;
}

// https://github.com/facebook/folly/blob/main/folly/FileUtil.cpp#L157
ssize_t preadFull(int fd, void* buf, size_t count, off_t offset) {
  return wrapFull(pread, fd, buf, count, offset);
}

ssize_t pwriteFull(int fd, const void* buf, size_t count, off_t offset) {
  return wrapFull(pwrite, fd, const_cast<void*>(buf), count, offset);
}

BlockDevice::~BlockDevice() { Close(); }

bool BlockDevice::IsBlkDev() {
  struct stat file_stat;
  if (::stat(dev_name_.c_str(), &file_stat)) {
    LOG(ERROR) << "failed to stat: " << dev_name_ << ", error: " << strerror(errno);
    return false;
  }
  return S_ISBLK(file_stat.st_mode);
}

bool BlockDevice::BlkDevGetSize() {
#ifdef BLKGETSIZE64
  if (::ioctl(dev_fd_direct_, BLKGETSIZE64, &dev_size_)) {
    LOG(ERROR) << "failed to BLKGETSIZE: " << dev_name_;
    return false;
  }
#elif defined(BLKGETSIZE)
  uint64_t sectors = 0;
  if (::ioctl(dev_fd_direct_, BLKGETSIZE, &sectors)) {
    LOG(ERROR) << "failed to BLKGETSIZE: " << dev_name;
    return false;
  }
  dev_size_ = sectors * 512ULL;
#else
  off_t s = ::lseek(dev_fd_direct_, 0, SEEK_END);
  if (s <= 0) {
    LOG(ERROR) << "failed to get dev size: " << dev_name;
    return false;
  }
  dev_size_ = static_cast<uint64_t>(s);
#endif
  LOG(DEBUG) << "get block device size: " << dev_size_ << " B";
  return true;
}

/*
 * Get logical sector size.
 *
 * This is the smallest unit the storage device can
 * address. It is typically 512 bytes.
 */
bool BlockDevice::BlkDevGetSectorSize() {
  if (::ioctl(dev_fd_direct_, BLKSSZGET, &sector_size_) < 0) {
    LOG(ERROR) << "failed to get dev block size: " << dev_name_;
    return false;
  }
  LOG(DEBUG) << "get block device blk size: " << sector_size_;
  return true;
}

bool BlockDevice::Open(const std::string &dev_name) {
  if (dev_name.empty() || dev_name == kBlockDeviceSda ||
      dev_name == kBlockDeviceVda || dev_name == kBlockDeviceCurrent ||
      dev_name == kBlockDeviceParent) {
    LOG(TRACE) << "device: " << dev_name << " not allowed, skip it";
    return false;
  }
  dev_name_ = dev_name;

  if (!IsBlkDev()) {
    LOG(ERROR) << "given path not blk device: " << dev_name_;
    errno = ENOTBLK;
    return false;
  }

  dev_fd_direct_ = ::open(dev_name.c_str(), kBlkOpenWithDirect);
  if (dev_fd_direct_ < 0) {
    LOG(ERROR) << "failed to open block device: " << dev_name
               << " errno: " << errno;
    return false;
  }
  dev_fd_cache_ = ::open(dev_name.c_str(), kBlkOpenWithoutDirect);
  if (dev_fd_cache_ < 0) {
    LOG(ERROR) << "failed to open block device: " << dev_name
               << " errno: " << errno;
    return false;
  }
  LOG(DEBUG) << "open block device " << dev_name << " success";

  if (!BlkDevGetSize()) {
    return false;
  }

  if (!BlkDevGetSectorSize()) {
    return false;
  }
  return true;
}

void BlockDevice::Close() {
  if (dev_fd_cache_ > 0) {
    Fsync();
    ::close(dev_fd_cache_);
    dev_fd_cache_ = -1;
  }
  if (dev_fd_direct_ > 0) {
    Fsync();
    ::close(dev_fd_direct_);
    dev_fd_direct_ = -1;
  }
  LOG(DEBUG) << "close block device " << dev_name_;
}

void BlockDevice::Sync() { ::sync(); }

int BlockDevice::Fsync() {
  int ret;
  if (dev_fd_direct_ > 0) {
    ret = ::fsync(dev_fd_direct_);
    if (ret < 0) {
      LOG(ERROR) << "fsync direct fd failed, errno: " << errno;
      return -1;
    }
  }
  if (dev_fd_cache_ > 0) {
    ret = ::fsync(dev_fd_cache_);
    if (ret < 0) {
      LOG(ERROR) << "fsync cache fd failed, errno: " << errno;
      return -1;
    }
  }
  return 0;
}

int64_t BlockDevice::PreadCache(void *buf, uint64_t len, off_t offset) {
  if ((offset + len) > dev_size_) [[unlikely]] {
    LOG(ERROR) << "device size is less than (offset + length), "
               << "offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return preadFull(dev_fd_cache_, buf, len, offset);
}

int64_t BlockDevice::PwriteCache(void *buf, uint64_t len, off_t offset) {
  if ((offset + len) > dev_size_) [[unlikely]] {
    LOG(ERROR) << "device size is less than (offset + length),"
               << " offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return pwriteFull(dev_fd_cache_, buf, len, offset);
}

int64_t BlockDevice::PreadDirect(void *buf, uint64_t len, off_t offset) {
  if ((offset + len) > dev_size_) [[unlikely]] {
    LOG(ERROR) << "device size is less than (offset + length), "
               << "offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return preadFull(dev_fd_direct_, buf, len, offset);
}

int64_t BlockDevice::PwriteDirect(void *buf, uint64_t len, off_t offset) {
  if ((offset + len) > dev_size_) [[unlikely]] {
    LOG(ERROR) << "device size is less than (offset + length),"
               << " offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return pwriteFull(dev_fd_direct_, buf, len, offset);
}
}