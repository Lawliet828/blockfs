#include "block_device.h"

// clang-format off
#include <fcntl.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/fs.h>
// #include <blkid/blkid.h>

#include "injection.h"
#include "logging.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

static const std::string kBlockDeviceSda = "/dev/sda";
static const std::string kBlockDeviceVda = "/dev/vda";
static const std::string kBlockDeviceCurrent = "/dev/.";
static const std::string kBlockDeviceParent = "/dev/..";

static const int32_t kIoNoMergedFlagOn = 2;
static const int32_t kIoNoMergedFlagOff = 0;
static constexpr uint32_t kIovMax = IOV_MAX;

static const int32_t kBlkOpenWithDirect =
    O_RDWR | O_LARGEFILE | O_DIRECT | O_CLOEXEC;
static const int32_t kBlkOpenWithoutDirect = O_RDWR | O_LARGEFILE | O_CLOEXEC;

inline void incr(int64_t /* n */) {}
inline void incr(int64_t n, off_t &offset) { offset += off_t(n); }

// Wrap call to read/pread/write/pwrite(fd, buf, count, offset?) to retry on
// incomplete reads / writes.  The variadic argument magic is there to support
// an additional argument (offset) for pread / pwrite; see the incr() functions
// above which do nothing if the offset is not present and increment it if it
// is.
template <class F, class... Offset>
int64_t WrapFull(F f, int fd, void *buf, uint64_t count, Offset... offset) {
  char *buffer = static_cast<char *>(buf);
  int64_t totalBytes = 0;
  int64_t r;
  do {
    r = f(fd, buffer, count, offset...);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      LOG(ERROR) << "return: " << r << " errno: " << errno;
      return r;
    }

    totalBytes += r;
    buffer += r;
    count -= r;
    incr(r, offset...);

  } while (r != 0 && count);  // 0 means EOF

  return totalBytes;
}

// Wrap call to readv/preadv/writev/pwritev(fd, iov, count, offset?) to
// retry on incomplete reads / writes.
template <class F, class... Offset>
int64_t WrapvFull(F f, int fd, iovec *iov, uint32_t count, Offset... offset) {
  int64_t totalBytes = 0;
  int64_t r;
  do {
    r = f(fd, iov, std::min<uint32_t>(count, kIovMax), offset...);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      return r;
    }

    if (r == 0) {
      break;  // EOF
    }

    totalBytes += r;
    incr(r, offset...);
    while (r != 0 && count != 0) {
      if (r >= ssize_t(iov->iov_len)) {
        r -= ssize_t(iov->iov_len);
        ++iov;
        --count;
      } else {
        iov->iov_base = static_cast<char *>(iov->iov_base) + r;
        iov->iov_len -= r;
        r = 0;
      }
    }
  } while (count);

  return totalBytes;
}

/**
 * Similar to readFull and preadFull above, wrappers around write() and
 * pwrite() that loop until all data is written.
 *
 * Generally, the write() / pwrite() system call may always write fewer bytes
 * than requested, just like read().  In certain cases (such as when writing to
 * a pipe), POSIX provides stronger guarantees, but not in the general case.
 * For example, Linux (even on a 64-bit platform) won't write more than 2GB in
 * one write() system call.
 *
 * Note that writevFull and pwritevFull require iov to be non-const, unlike
 * writev and pwritev.  The contents of iov after these functions return
 * is unspecified.
 *
 * These functions return -1 on error, or the total number of bytes written
 * (which is always the same as the number of requested bytes) on success.
 */
int64_t ReadFull(int fd, void *buf, uint64_t size) {
  return WrapFull(::read, fd, buf, size);
}

int64_t PreadFull(int fd, void *buf, uint64_t size, off_t offset) {
  return WrapFull(::pread, fd, buf, size, offset);
}

int64_t WriteFull(int fd, const void *buf, uint64_t size) {
  return WrapFull(::write, fd, const_cast<void *>(buf), size);
}

int64_t PwriteFull(int fd, const void *buf, uint64_t size, off_t offset) {
  return WrapFull(::pwrite, fd, const_cast<void *>(buf), size, offset);
}

int64_t ReadvFull(int fd, iovec *iov, uint64_t size) {
  return WrapvFull(::readv, fd, iov, size);
}

int64_t PreadvFull(int fd, iovec *iov, uint64_t size, off_t offset) {
  return WrapvFull(::preadv, fd, iov, size, offset);
}

int64_t WritevFull(int fd, iovec *iov, uint64_t size) {
  return WrapvFull(::writev, fd, iov, size);
}
// RWF_HIPRI
// http://manpages.ubuntu.com/manpages/bionic/man2/writev.2.html
int64_t PwritevFull(int fd, iovec *iov, uint64_t size, off_t offset) {
  return WrapvFull(::pwritev, fd, iov, size, offset);
}

BlockDevice::BlockDevice(bool io_no_merges) : io_no_merges_(io_no_merges) {}

BlockDevice::~BlockDevice() { Close(); }

const char *BlockDevice::sysfsdir() const { return "/sys"; }

bool BlockDevice::IsBlkDev() {
  struct stat st;
  if (::stat(dev_name_.c_str(), &st)) {
    LOG(ERROR) << "failed to stat: " << dev_name_;
    return false;
  }
  return S_ISBLK(st.st_mode);
}

dev_t BlockDevice::BlkDevId() const {
  struct stat st;
  int r;
  if (dev_fd_direct_ >= 0) {
    r = ::fstat(dev_fd_direct_, &st);
  } else {
    char path[PATH_MAX] = {0};
    ::snprintf(path, sizeof(path), "/dev/%s", dev_name_.c_str());
    r = ::stat(path, &st);
  }
  if (r < 0) {
    return -errno;
  }
  dev_t id = S_ISBLK(st.st_mode) ? st.st_rdev : st.st_dev;
  return id;
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

/*
 * Return the alignment status of a device
 */
bool BlockDevice::BlkDevIsMisaligned() {
#ifdef BLKALIGNOFF
  int32_t aligned = 0;
  if (::ioctl(dev_fd_direct_, BLKALIGNOFF, &aligned) < 0)
    return false; /* probably kernel < 2.6.32 */
  /* Note that kernel returns -1 as alignment offset if no compatible
   * sizes and alignments exist for stacked devices */
  return aligned != 0 ? true : false;
#else
  return false;
#endif
}

/**
 * get a block device property as a string
 *
 * store property in *val, up to maxlen chars
 * return 0 on success
 * return negative error on error
 */
int64_t BlockDevice::GetStringProperty(const char *property, char *val,
                                       size_t maxlen) const {
  char filename[PATH_MAX], wd[PATH_MAX];
  const char *dev = nullptr;

  if (dev_fd_direct_ >= 0) {
    // sysfs isn't fully populated for partitions, so we need to lookup the
    // sysfs entry for the underlying whole disk.
    if (int r = WholeDisk(wd, sizeof(wd)); r < 0) return r;
    dev = wd;
  } else {
    dev = dev_name_.c_str();
  }
  if (::snprintf(filename, sizeof(filename), "%s/block/%s/%s", sysfsdir(), dev,
                 property) >= static_cast<int>(sizeof(filename))) {
    return -ERANGE;
  }

  FILE *fp = ::fopen(filename, "r");
  if (fp == NULL) {
    return -errno;
  }

  int r = 0;
  if (::fgets(val, maxlen - 1, fp)) {
    // truncate at newline
    char *p = val;
    while (*p && *p != '\n') ++p;
    *p = 0;
  } else {
    r = -EINVAL;
  }
  ::fclose(fp);
  return r;
}

/**
 * get a block device property
 *
 * return the value (we assume it is positive)
 * return negative error on error
 */
int64_t BlockDevice::GetIntProperty(const char *property) const {
  char buff[256] = {0};
  int r = GetStringProperty(property, buff, sizeof(buff));
  if (r < 0) return r;
  // take only digits
  for (char *p = buff; *p; ++p) {
    if (!::isdigit(*p)) {
      *p = 0;
      break;
    }
  }
  char *endptr = 0;
  r = ::strtoll(buff, &endptr, 10);
  if (endptr != buff + ::strlen(buff)) r = -EINVAL;
  return r;
}

bool BlockDevice::SupportDiscard() const {
  return GetIntProperty("queue/discard_granularity") > 0;
}

bool BlockDevice::IsRotational() const {
  return GetIntProperty("queue/rotational") > 0;
}

int BlockDevice::GetDev(char *dev, size_t max) const {
  return GetStringProperty("dev", dev, max);
}

int BlockDevice::GetVendor(char *vendor, size_t max) const {
  return GetStringProperty("device/device/vendor", vendor, max);
}

int BlockDevice::GetModel(char *model, size_t max) const {
  return GetStringProperty("device/model", model, max);
}

int BlockDevice::GetSerial(char *serial, size_t max) const {
  return GetStringProperty("device/serial", serial, max);
}

int BlockDevice::WholeDisk(char *device, size_t max) const {
  dev_t id = BlkDevId();
  if (id < 0) return -EINVAL;  // hrm.

  // int r = blkid_devno_to_wholedisk(id, device, max, nullptr);
  // if (r < 0) {
  //   return -EINVAL;
  // }
  return 0;
}

int BlockDevice::WholeDisk(std::string *s) const {
  char out[PATH_MAX] = {0};
  int r = WholeDisk(out, sizeof(out));
  if (r < 0) {
    return r;
  }
  *s = out;
  return r;
}

// 如何改变只读的system或者根文件系统为读写
// https://blog.csdn.net/zhenwenxian/article/details/6658494
// http://www.360doc.com/content/16/0105/09/25419505_525574037.shtml
// https://blog.51cto.com/9291927/1796547

// echo 2 > /sys/block/vdb/queue/nomerges
bool BlockDevice::SetIONomerges() {
  std::string path = "/sys/block/" +
                     std::string(::basename(dev_name_.c_str())) +
                     "/queue/nomerges";
  int32_t write_flag = io_no_merges_ ? kIoNoMergedFlagOn : kIoNoMergedFlagOff;
  int32_t check_flag = -1;
  FILE *fp = nullptr;
  if (io_no_merges_) {
    LOG(DEBUG) << "master node need to set nomerges flag";
    fp = ::fopen(path.c_str(), "w+");
    if (!fp) {
      LOG(ERROR) << "failed to open path: " << path << " errno:" << errno;
      return false;
    }
    ::fprintf(fp, "%d\n", write_flag);
    ::fflush(fp);
    ::rewind(fp);
  } else {
    LOG(DEBUG) << "slave node no need to set nomerges, only check";
    fp = ::fopen(path.c_str(), "r+");
    if (!fp) {
      LOG(ERROR) << "failed to open path: " << path << " errno:" << errno;
      return false;
    }
  }
  // check whether is success
  if (::fscanf(fp, "%d", &check_flag) == 1 && check_flag == write_flag) {
    LOG(DEBUG) << "set " << dev_name_ << " nomerges " << write_flag
               << " successfully";
    ::fclose(fp);
    return true;
  }
  LOG(ERROR) << "failed to set nomerges for:" << dev_name_
             << " flag:" << write_flag;
  ::fclose(fp);
  return false;
}

bool BlockDevice::Open(const std::string &dev_name) {
  if (dev_name.empty() || dev_name == kBlockDeviceSda ||
      dev_name == kBlockDeviceVda || dev_name == kBlockDeviceCurrent ||
      dev_name == kBlockDeviceParent) {
    LOG(DEBUG) << "device: " << dev_name << " not allowed, skip it";
    return false;
  }
  dev_name_ = dev_name;

  if (!IsBlkDev()) {
    LOG(ERROR) << "given path not blk device: " << dev_name_;
    errno = ENOTBLK;
    return false;
  }

  // blkid | grep /dev/vdb | wc -l
  // cat /proc/mounts | grep /dev/vdb | wc -l
  // /proc/mounts 已经被mount过
  // sudo blkid 查看是否做了文件系统

  // set and check flag before open device
  // if (!SetIONomerges()) {
  //   return false;
  // }

  // 不需要额外的access检查,如果不存在的话会报错
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

int BlockDevice::ReOpen(const std::string &dev_name) {
  Close();
  return Open(dev_name);
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

int BlockDevice::Fdatasync() {
  // indicate 'Force Unit Access'
  return Fsync();
}

int BlockDevice::SyncFS() { return Fsync(); }

enum TRIM_ACT {
  ACT_DISCARD = 0, /* default */
  ACT_ZEROOUT,
  ACT_SECURE
};

#ifdef CLOCK_MONOTONIC_RAW
#define UL_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#else
#define UL_CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

int gettime_monotonic(struct timeval *tv) {
#ifdef CLOCK_MONOTONIC
  /* Can slew only by ntp and adjtime */
  int ret;
  struct timespec ts;

  /* Linux specific, can't slew */
  if (!(ret = ::clock_gettime(UL_CLOCK_MONOTONIC, &ts))) {
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
  }
  return ret;
#else
  return ::gettimeofday(tv, NULL);
#endif
}

static void DebugDiscardStats(int act, uint64_t stats[]) {
  switch (act) {
    case ACT_ZEROOUT:
      LOG(DEBUG) << "zero-filled " << stats[1] << " bytes from the offset "
                 << stats[0];
      break;
    case ACT_SECURE:
    case ACT_DISCARD:
      LOG(DEBUG) << "discarded " << stats[1] << " bytes from the offset "
                 << stats[0];
      break;
  }
}
// trim
int BlockDevice::Discard(uint64_t offset, uint64_t len) {
  if (!support_discard_) {
    LOG(ERROR) << "discard has not been supported yet";
    return -1;
  }

  struct timeval now, last;
  uint64_t end, step, range[2], stats[2];
  range[0] = 0;
  range[1] = UINT64_MAX;
  step = 0;
  TRIM_ACT act = ACT_DISCARD;

  /* check offset alignment to the sector size */
  if (range[0] % sector_size_) {
    LOG(ERROR) << "offset " << range[0]
               << " is not aligned "
                  "to sector size "
               << sector_size_;
    return -1;
  }

  /* is the range end behind the end of the device ?*/
  if (range[0] > dev_size_) {
    LOG(ERROR) << "offset " << range[0] << " is greater than device size";
    return -1;
  }
  end = range[0] + range[1];
  if (end < range[0] || end > dev_size_) {
    end = dev_size_;
  }
  range[1] = (step > 0) ? step : end - range[0];

  /* check offset alignment to the sector size */
  if (range[1] % sector_size_) {
    LOG(ERROR) << "offset " << range[1]
               << " is not aligned "
                  "to sector size "
               << sector_size_;
    return -1;
  }

  stats[0] = range[0], stats[1] = 0;
  gettime_monotonic(&last);
  for (/* nothing */; range[0] < end; range[0] += range[1]) {
    if (range[0] + range[1] > end) range[1] = end - range[0];

    switch (act) {
      case ACT_ZEROOUT:
        if (::ioctl(dev_fd_direct_, BLKZEROOUT, &range)) {
          LOG(ERROR) << "BLKZEROOUT ioctl failed, errno: " << errno;
          return -1;
        }
        break;
      case ACT_SECURE:
        if (::ioctl(dev_fd_direct_, BLKSECDISCARD, &range)) {
          LOG(ERROR) << "BLKSECDISCARD ioctl failed, errno: " << errno;
          return -1;
        }
        break;
      case ACT_DISCARD:
        // uint64_t range[2] = {offset, len};
        // return ::ioctl(dev_fd_direct_, BLKDISCARD, range);
        if (::ioctl(dev_fd_direct_, BLKDISCARD, &range)) {
          LOG(ERROR) << "BLKDISCARD ioctl failed, errno: " << errno;
          return -1;
        }
        break;
    }
    stats[1] += range[1];
    /* reporting progress at most once per second */
    if (verbose_discard_ && step) {
      gettime_monotonic(&now);
      if (now.tv_sec > last.tv_sec &&
          (now.tv_usec >= last.tv_usec || now.tv_sec > last.tv_sec + 1)) {
        DebugDiscardStats(act, stats);
        stats[0] += stats[1], stats[1] = 0;
        last = now;
      }
    }
  }

  if (verbose_discard_ && stats[1]) DebugDiscardStats(act, stats);

  return 0;
}

int64_t BlockDevice::ReadCache(void *buf, uint64_t len) {
  return ReadFull(dev_fd_cache_, buf, len);
}

int64_t BlockDevice::PreadCache(void *buf, uint64_t len, off_t offset) {
  if (unlikely((offset + len) > dev_size_)) {
    LOG(ERROR) << "device size is less than (offset + length), "
               << "offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return PreadFull(dev_fd_cache_, buf, len, offset);
}

int64_t BlockDevice::WriteCache(void *buf, uint64_t len) {
  return WriteFull(dev_fd_cache_, buf, len);
}

int64_t BlockDevice::PwriteCache(void *buf, uint64_t len, off_t offset) {
  if (unlikely((offset + len) > dev_size_)) {
    LOG(ERROR) << "device size is less than (offset + length),"
               << " offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return PwriteFull(dev_fd_cache_, buf, len, offset);
}

int64_t BlockDevice::ReadDirect(void *buf, uint64_t len) {
  return ReadFull(dev_fd_direct_, buf, len);
}

int64_t BlockDevice::PreadDirect(void *buf, uint64_t len, off_t offset) {
  if (unlikely((offset + len) > dev_size_)) {
    LOG(ERROR) << "device size is less than (offset + length), "
               << "offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return PreadFull(dev_fd_direct_, buf, len, offset);
}

int64_t BlockDevice::WriteDirect(void *buf, uint64_t len) {
  return WriteFull(dev_fd_direct_, buf, len);
}

int64_t BlockDevice::PwriteDirect(void *buf, uint64_t len, off_t offset) {
  if (unlikely((offset + len) > dev_size_)) {
    LOG(ERROR) << "device size is less than (offset + length),"
               << " offset: " << offset << " length: " << len
               << " dev_size: " << dev_size_;
    return -1;
  }
  return PwriteFull(dev_fd_direct_, buf, len, offset);
}
}  // namespace blockfs
}  // namespace udisk