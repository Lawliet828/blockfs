#include "super_block.h"

#include <uuid/uuid.h>

#include "crc.h"
#include "file_system.h"
#include "logging.h"

SuperBlock::SuperBlock() {}
SuperBlock::~SuperBlock() { buffer_.reset(); }

/**
 * get the total super metadata on the shared udisk
 * only used to prefetch super meta when blockfs startup
 *
 * \param void
 *
 * \return success or failed
 */
bool SuperBlock::InitializeMeta() {
  SuperBlockMeta *meta = reinterpret_cast<SuperBlockMeta *>(base_addr());
  uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                       kSuperBlockSize - sizeof(meta->crc_));
  if (unlikely(meta->crc_ != crc)) {
    LOG(ERROR) << "super block crc error, read:" << meta->crc_
               << " cal: " << crc;
    return false;
  }
  if (unlikely(meta->magic_ != kBlockFsMagic)) {
    LOG(ERROR) << "super block magic error, read:" << meta->magic_
               << " wanted: " << kBlockFsMagic;
    return false;
  }
  LOG(DEBUG) << "read super block success";
  return true;
}

bool SuperBlock::FormatAllMeta() {
  buffer_ = std::make_shared<AlignBuffer>(
      kSuperBlockSize, FileSystem::Instance()->dev()->block_size());

  // init base addr beacause other handles needed later
  set_base_addr(buffer_->data());

  SuperBlockMeta *meta = reinterpret_cast<SuperBlockMeta *>(buffer_->data());

  /* BlockFS的常量定义 */
  uuid_t uuid;
  ::uuid_generate(uuid);
  ::uuid_unparse(uuid, meta->uuid_);
  meta->magic_ = kBlockFsMagic;
  meta->max_file_num = kBlockFsMaxFileNum;
  meta->max_support_udisk_size_ = kBlockFsMaxUDiskSize;
  meta->block_size_ = kBlockSize;
  meta->max_dir_name_len_ = kBlockFsMaxDirNameLen;
  meta->max_file_name_len_ = kBlockFsMaxFileNameLen;
  meta->dir_meta_size_ = kBlockFsDirMetaSize;
  meta->file_meta_size_ = kBlockFsFileMetaSize;
  meta->file_block_meta_size_ = kBlockFsFileBlockMetaSize;

  /* 2. 超级块区域: 起始位置 0 - 4096 */
  meta->super_block_offset_ = kSuperBlockOffset;
  meta->super_block_size_ = kSuperBlockSize;

  /* 3. 文件夹区域: 起始位置 4096 - meta->file_meta_offset_ */
  meta->dir_meta_offset_ = kDirMetaOffset;
  meta->dir_meta_total_size_ =
      meta->dir_meta_size_ * meta->max_file_num;
  meta->dir_meta_total_size_ =
      ROUND_UP(meta->dir_meta_total_size_, kBlockFsPageSize);

  /* 4. 文件区域: 起始位置 dir_meta结尾 - meta->file_block_meta_offset_ */
  meta->file_meta_offset_ = meta->dir_meta_offset_ + meta->dir_meta_total_size_;
  meta->file_meta_total_size_ =
      meta->file_meta_size_ * meta->max_file_num;
  meta->file_meta_total_size_ =
      ROUND_UP(meta->file_meta_total_size_, kBlockFsPageSize);

  /* 5. 文件块区域: 起始位置 file_meta结尾 - xxx */
  meta->file_block_meta_offset_ =
      meta->file_meta_offset_ + meta->file_meta_total_size_;

  // 一个超大文件(无限接近128T),其他都是小文件
  uint64_t tmpNum1 = meta->max_file_num - 1;
  uint64_t tmpSize1 = tmpNum1 * meta->file_block_meta_size_;
  uint64_t tmpNum2 = ALIGN_UP(meta->max_support_udisk_size_ - tmpSize1,
                              (meta->block_size_ * kFileBlockCapacity));

  uint64_t tmpSize2 = tmpNum2 * meta->file_block_meta_size_;
  meta->file_block_meta_total_size_ = tmpSize1 + tmpSize2;
  meta->file_block_meta_total_size_ =
      ROUND_UP((tmpSize1 + tmpSize2), kBlockFsPageSize);
  meta->max_file_block_num = tmpNum1 + tmpNum2;

  meta->block_data_start_offset_ =
      meta->file_block_meta_offset_ + meta->file_block_meta_total_size_;

  // 元数据区域大小保证4M对齐,也就是数据区域的起始位置是4M对齐的
  meta->block_data_start_offset_ =
      ROUND_UP(meta->block_data_start_offset_, kBlockSize);

  meta->device_size = FileSystem::Instance()->dev()->dev_size();
  uint64_t free_udisk_size =
      meta->device_size - meta->block_data_start_offset_;
  meta->curr_block_num = free_udisk_size / kBlockSize;

  // 最大支持12T的block个数
  meta->max_support_block_num_ =
      (meta->max_support_udisk_size_ - meta->block_data_start_offset_) /
      kBlockSize;

  meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                     kSuperBlockSize - sizeof(meta->crc_));
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      meta, kSuperBlockSize, kSuperBlockOffset);
  if (ret != static_cast<int64_t>(kSuperBlockSize)) {
    LOG(ERROR) << "write super block error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all super block success";
  return true;
}

/**
 * set uxdb mount point
 *
 * \param mount point
 *
 * \return success or failed
 */
bool SuperBlock::set_uxdb_mount_point(const std::string &path) {
  LOG(INFO) << "UXDB root path: " << path;

  std::string root_path = path;
  std::string mount_point = uxdb_mount_point();
  if (root_path[root_path.size() - 1] != '/') {
    root_path += "/";
  }
  if (root_path.size() >= kUXDBMountPrefixMaxLen) {
    LOG(ERROR) << "mount point path too long, size: " << root_path.size();
    return false;
  }
  // 检查已经配置过MountPoint检查设置的是否一致
  // 当前系统要求挂载点不能更改
  if (mount_point.size() > 0) {
    // 读取的即将设置的挂载点一致
    if (root_path == mount_point) {
      LOG(INFO) << "mount point has been configured";
      return true;
    } else {
      LOG(ERROR) << "mount point  not matched, rootpath: " << root_path
                 << " uxdb_mount_point: " << mount_point;
      return false;
    }
  }
  ::memset(meta()->uxdb_mount_point_, 0, sizeof(meta()->uxdb_mount_point_));
  ::memcpy(meta()->uxdb_mount_point_, root_path.c_str(), root_path.size());
  LOG(INFO) << "UXDB mount point: " << meta()->uxdb_mount_point_;

  if (!WriteMeta()) {
    return false;
  }
  LOG(INFO) << "write mount point: " << meta()->uxdb_mount_point_ << " success";
  return true;
}

/**
 * check path whether is uxdb mount point
 *
 * \param path
 *
 * \return yes or no
 */
bool SuperBlock::is_mount_point(const std::string &path) noexcept {
  std::string mount_point = uxdb_mount_point();
  if (path == mount_point) {
    return true;
  }
  std::string tmp_path = path;
  if (tmp_path[tmp_path.size() - 1] != '/') {
    tmp_path += '/';
  }
  if (tmp_path == mount_point) {
    return true;
  }
  return false;
}

/**
 * Check the legality of the file/directory mounting path
 *
 * \param path : absolute directory
 */
bool SuperBlock::CheckMountPoint(const std::string &path, bool isFile) {
  if (path.empty()) [[unlikely]] {
    LOG(ERROR) << "path cannot be empty";
    errno = EINVAL;
    return false;
  }
  std::string sub = meta()->uxdb_mount_point_;
  LOG(INFO) << "check target path: " << path << " mount point: " << sub;
  if (path.size() < sub.size()) {
    LOG(ERROR) << "path less than mount moint: " << path;
    errno = EXDEV;
    return false;
  }
  if (path.compare(0, sub.size(), sub) != 0) {
    LOG(ERROR) << "path not startwith mount prefix: " << path;
    errno = EXDEV;
    return false;
  }
  if (isFile) {
    if (path[path.size() - 1] == '/') [[unlikely]] {
      LOG(ERROR) << "file cannot endwith dir separator: " << path;
      errno = ENOTDIR;
      return false;
    }
  }
  errno = 0;
  return true;
}

/**
 * dump super metadata info
 *
 * \param void
 *
 * \return void
 */
void SuperBlock::Dump() noexcept {
  LOG(INFO) << "super block info:"
            << "\n"
            << "crc: " << meta()->crc_ << "\n"
            << "uuid: " << meta()->uuid_ << "\n"
            << "magic: " << meta()->magic_ << "\n"
            << "seq_no: " << meta()->seq_no_ << "\n"
            << "max_file_num: " << meta()->max_file_num << "\n"
            << "max_udisk_size: " << meta()->max_support_udisk_size_ << "\n"
            << "block_size: " << meta()->block_size_ << "\n"
            << "max_support_block_num_: " << meta()->max_support_block_num_
            << "\n"
            << "max_dir_name_len: " << meta()->max_dir_name_len_ << "\n"
            << "max_file_name_len: " << meta()->max_file_name_len_ << "\n"
            << "dir_meta_size: " << meta()->dir_meta_size_ << "\n"
            << "file_meta_size: " << meta()->file_meta_size_ << "\n"
            << "file_block_meta_size: " << meta()->file_block_meta_size_ << "\n"
            << "super_block_size: " << meta()->super_block_size_ << "\n"
            << "dir_meta_total_size: " << meta()->dir_meta_total_size_ << "\n"
            << "file_meta_total_size: " << meta()->file_meta_total_size_ << "\n"
            << "file_block_meta_num: " << meta()->max_file_block_num
            << "\n"
            << "file_block_meta_total_size: "
            << meta()->file_block_meta_total_size_ << "\n"
            << "super_block_offset: " << meta()->super_block_offset_ << "\n"
            << "dir_meta_offset: " << meta()->dir_meta_offset_ << "\n"
            << "file_meta_offset: " << meta()->file_meta_offset_ << "\n"
            << "file_block_meta_offset: " << meta()->file_block_meta_offset_
            << "\n"
            << "data_start_offset: " << meta()->block_data_start_offset_ << "\n"
            << "device_size: " << meta()->device_size << "\n"
            << "curr_block_num: " << meta()->curr_block_num << "\n"
            << "uxdb_mount_point: " << meta()->uxdb_mount_point_;
}

/**
 * dump super metadata info to outer file
 *
 * \param void
 *
 * \return void
 */
void SuperBlock::Dump(const std::string &file_name) noexcept {}

bool SuperBlock::WriteMeta() {
  meta()->crc_ =
      Crc32(reinterpret_cast<uint8_t *>(base_addr()) + sizeof(meta()->crc_),
            kSuperBlockSize - sizeof(meta()->crc_));
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      base_addr(), kSuperBlockSize, kSuperBlockOffset);
  if (ret != static_cast<int64_t>(kSuperBlockSize)) {
    LOG(ERROR) << "write super block error size:" << ret;
    return false;
  }
  LOG(INFO) << "write super block success";
  return true;
}