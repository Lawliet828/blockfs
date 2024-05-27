
#include "shm_manager.h"

#include <dirent.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "file_store_udisk.h"
#include "logging.h"

namespace udisk::blockfs {

// https://blog.csdn.net/ababab12345/article/details/102931841

// blockfs_shm_xxx (uuid)
static const std::string kMetaShmNamePrefix = "bfs_shm_";

ShmManager::ShmManager() { shm_addr_ = static_cast<char *>(MAP_FAILED); }

ShmManager::~ShmManager() { MemUnMap(); }

bool ShmManager::PrefetchSuperMeta(SuperBlockMeta &meta) {
  ::memset(&meta, 0, sizeof(SuperBlockMeta));
  int64_t ret = FileSystem::Instance()->dev()->PreadCache(&meta, kSuperBlockSize,
                                                         kSuperBlockOffset);
  if (unlikely(ret != kSuperBlockSize)) {
    LOG(ERROR) << "read super block error size:" << ret;
    return false;
  }
  return true;
}

/**
 * get the total super metadata on the shared udisk
 * only used to prefetch super meta when blockfs startup
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::PrefetchSuperMeta() {
  if (!PrefetchSuperMeta(super_)) {
    return false;
  }
  uint32_t crc =
      Crc32(reinterpret_cast<uint8_t *>(&super_) + sizeof(super_.crc_),
            kSuperBlockSize - sizeof(super_.crc_));
  if (super_.crc_ != crc) [[unlikely]] {
    LOG(ERROR) << "super block crc error, read:" << super_.crc_
               << " cal: " << crc;
    return false;
  }
  if (super_.magic_ != kBlockFsMagic) [[unlikely]] {
    LOG(ERROR) << "super block magic error, read:" << super_.magic_
               << " wanted: " << kBlockFsMagic;
    return false;
  }
  LOG(DEBUG) << "read super block success";
  LOG(DEBUG) << "mount point: " << super_.uxdb_mount_point_;
  LOG(DEBUG) << "block_data_start_offset: " << super_.block_data_start_offset_;

  shm_size_ = super_.block_data_start_offset_;
  shm_name_ = kMetaShmNamePrefix + super_.uuid_;

  LOG(DEBUG) << "shm name: " << shm_name_;
  LOG(DEBUG) << "shm size: " << shm_size_;

  struct sysinfo si;
  ::sysinfo(&si);
  LOG(DEBUG) << "Memory usage:"
             << "\n\t Total RAM: " << si.totalram
             << "\n\t Free RAM: " << si.freeram
             << "\n\t Shared RAM: " << si.sharedram
             << "\n\t Needed shared memory: " << shm_size_;

  return true;
}

/**
 * open shm using posix shm
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::ShmOpen() {
  if (shm_size_ == 0) [[unlikely]] {
    LOG(ERROR) << "shm size cannot be zero";
    return false;
  }

  shm_fd_ = ::shm_open(shm_name_.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0666);
  if (shm_fd_ < 0) [[unlikely]] {
    LOG(ERROR) << "posix shm open failed, errno: " << errno;
    return false;
  }
  return true;
}

/**
 * set shm size and mmap shm memory
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::MemMap() {
  if (::ftruncate(shm_fd_, shm_size_) < 0) {
    LOG(ERROR) << "ftruncate shm fd failed, errno: " << errno;
    return false;
  }
  shm_addr_ = (char *)::mmap(0, shm_size_, PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_SHARED | MAP_FIXED, shm_fd_, 0);
  if (shm_addr_ == MAP_FAILED) {
    LOG(ERROR) << "mmap shm fd failed, errno: " << errno;
    return false;
  }
  ::close(shm_fd_);
  shm_fd_ = -1;

  // 每次重启都清除?
  // ::memset(shm_addr_, 0, shm_size_);
  return true;
}

/**
 * unmmap shm memory
 * https://www.cnblogs.com/Marineking/p/3747217.html
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::MemUnMap() {
  if (shm_fd_ > 0) {
    ::close(shm_fd_);
    shm_fd_ = -1;
  }
  return true;
}

bool ShmManager::NewPosixAlignMem() {
  if (shm_size_ == 0) {
    LOG(ERROR) << "shm size cannot be zero";
    return false;
  }
  LOG(DEBUG) << "using posix mem aligned meta";
  buffer_ = std::make_shared<AlignBuffer>(
      shm_size_, FileSystem::Instance()->dev()->block_size());

  shm_addr_ = buffer_->data();
  return true;
}

/**
 * read all metadata into shm memory
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::ReadAllMeta() {
  ::memset(shm_addr_, 0, shm_size_);
  int64_t ret =
      FileSystem::Instance()->dev()->PreadDirect(shm_addr_, shm_size_, 0);
  if (unlikely(ret != shm_size_)) {
    LOG(ERROR) << "read all meta error size: " << ret;
    return false;
  }
  LOG(DEBUG) << "read all meta into shm success";
  return true;
}

/**
 * Initialize shm management and read all metadata
 */
bool ShmManager::Initialize(bool create) {
  if (!PrefetchSuperMeta()) {
    return false;
  }

  if (create) {
    if (!NewPosixAlignMem()) {
      return false;
    }
  }

  if (!ReadAllMeta()) {
    return false;
  }
  RegistMetaBaseAddr();
  LOG(DEBUG) << "init and register meta success";
  return true;
}

/**
 * delete shm memory, never called!!!
 *
 * \param void
 *
 * \return success or failed
 */
bool ShmManager::Destroy() {
  int ret = ::shm_unlink(shm_name_.c_str());
  if (ret != 0) {
    LOG(ERROR) << "posix shm unlink failed, errno: " << errno;
    return false;
  }
  return true;
}

/**
 * register meta handle shm base adder
 *
 * \param void
 *
 * \return void
 */
void ShmManager::RegistMetaBaseAddr() {
  FileSystem::Instance()->super()->set_base_addr(shm_addr_ + kSuperBlockOffset);
  FileSystem::Instance()->dir_handle()->set_base_addr(shm_addr_ +
                                                     super_.dir_meta_offset_);
  FileSystem::Instance()->file_handle()->set_base_addr(shm_addr_ +
                                                      super_.file_meta_offset_);
  FileSystem::Instance()->file_block_handle()->set_base_addr(
      shm_addr_ + super_.file_block_meta_offset_);
}

/**
 * clean last dirty shm
 *
 * \param last uuid
 *
 * \return void
 */
void ShmManager::CleanupShareMemory(const std::string &uuid) {
  DIR *shmdir;
  int dir_fd;
  struct dirent *entry;

  LOG(DEBUG) << "clean dirty: /dev/shm/" << kMetaShmNamePrefix << uuid;

  shmdir = ::opendir("/dev/shm");
  if (!shmdir) {
    LOG(FATAL) << "/dev/shm not exists";
    return;
  }
  dir_fd = ::dirfd(shmdir);
  entry = ::readdir(shmdir);
  while (entry != nullptr) {
    if (::strstr(entry->d_name, uuid.c_str())) {
      LOG(DEBUG) << "delete /dev/shm/" << kMetaShmNamePrefix << uuid;
      ::unlinkat(dir_fd, entry->d_name, 0);
      closedir(shmdir);
      return;
    }
    entry = ::readdir(shmdir);
  }
  ::closedir(shmdir);
}

void ShmManager::CleanupDirtyShareMemory() {
  SuperBlockMeta meta;
  bool success = ShmManager::PrefetchSuperMeta(meta);
  if (success && ::strlen(meta.uuid_) > 0) {
    ShmManager::CleanupShareMemory(meta.uuid_);
  } else {
    LOG(DEBUG) << "threre is no dirty blockfs shm";
  }
}

}