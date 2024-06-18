#include "file.h"

#include <assert.h>
#include <stdarg.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>

#include "file_system.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

bool File::WriteMeta(int32_t fh) {
  FileMeta *meta = reinterpret_cast<FileMeta *>(
      FileSystem::Instance()->file_handle()->base_addr() +
      FileSystem::Instance()->super_meta()->file_meta_size_ * fh);
  uint32_t align_index =
      FileSystem::Instance()->file_handle()->PageAlignIndex(fh);
  uint64_t offset =
      FileSystem::Instance()->super_meta()->file_meta_size_ * align_index;
  void *align_meta =
      static_cast<char *>(FileSystem::Instance()->file_handle()->base_addr()) +
      offset;
  meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                     FileSystem::Instance()->super_meta()->file_meta_size_ -
                         sizeof(meta->crc_));
  uint32_t crc = meta->crc_;
  SPDLOG_INFO("write file meta, name: {} fh: {} align_index: {} crc: {}", meta->file_name_, fh, align_index, crc);
  // TODO: 需要全局统一枷锁
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      align_meta, kBlockFsPageSize,
      FileSystem::Instance()->super_meta()->file_meta_offset_ + offset);
  if (ret != kBlockFsPageSize) [[unlikely]] {
    LOG(ERROR) << "write file meta, name: " << meta->file_name_ << " fh: " << fh
               << " error size:" << ret << " need:" << kBlockFsPageSize;
    return false;
  }
  return true;
}

void File::ClearMeta(FileMeta *meta) {
  int32_t fh = meta->fh_;
  SPDLOG_INFO("clear file meta, name: {} fh: {}", meta->file_name_, fh);
  meta->used_ = false;
  meta->is_temp_ = false;
  meta->size_ = 0;
  meta->mode_ = 0;
  meta->nlink_ = 0;
  meta->atime_ = 0;
  meta->mtime_ = 0;
  meta->ctime_ = 0;
  // 清标志位即可,申请时需要清理
  // dh在申请新文件时候 肯定会被覆盖
  // 后面回收内存文件需要
  // meta->seq_no_ = kReservedUnusedSeq;
  // meta->dh_ = -1;
  meta->child_fh_ = -1;
  meta->parent_fh_ = -1;
  meta->parent_size_ = -1;
}

ParentFilePtr ParentFile::NewParentFile(FileMeta *meta, uint64_t offset,
                                        FileBlockMap &fbs, bool tmpfile) {
  return std::make_shared<ParentFile>(meta, offset, fbs, tmpfile);
}

ParentFile::ParentFile(FileMeta *meta, uint64_t offset, FileBlockMap &fbs,
                       bool tmpfile)
    : meta_(meta), offset_(offset), tmp_file_(tmpfile) {
  fbs.swap(fb_maps_);
}

ParentFile::~ParentFile() {
  LOG(INFO) << "parent file delete, fh: " << meta_->fh_;
}

bool ParentFile::Recycle() {
  // block_num后面的都需要回收
  uint32_t block_num = offset_ / kBlockSize;
  // 释放blockid
  uint32_t block_id_index = 0;

  // TODO: 从最后开始释放
  for (uint32_t i = 0; i < fb_maps_.size(); ++i) {
    const FileBlockPtr &fb = fb_maps_[i];
    for (uint32_t j = 0; j < fb->used_block_num(); ++j) {
      // 前面被子文件继承的Block不能被释放
      if (block_id_index < block_num) {
        ++block_id_index;
      } else {
        LOG(INFO) << meta_->file_name_ << " put block id "
                  << fb->meta()->block_id_[j] << " to freelist";
        FileSystem::Instance()->block_handle()->PutFreeBlockIdLock(
            fb->meta()->block_id_[j]);
      }
    }
    // TODO: FileBlock需要释放
    fb->ReleaseMyself();
  }
  LOG(INFO) << "recycle file name: " << meta_->file_name_
            << " fh: " << meta_->fh_;

  // 释放file
  File::ClearMeta(meta_);
  if (!File::WriteMeta(meta_->fh_)) {
    return false;
  }

  FileSystem::Instance()->file_handle()->AddFile2FreeNolock(meta_->fh_);
  return true;
}

int File::rename(const std::string &to) {
  bool success =
      FileSystem::Instance()->file_handle()->RunInMetaGuard([this, to] {
        SPDLOG_INFO("rename file {} -> {}", file_name(), to);
        // 先把之前的文件映射去掉
        const DirectoryPtr &old_dir =
            FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dh());
        if (!old_dir) {
          return false;
        }

        std::string dir_name = GetDirName(to);
        const DirectoryPtr &new_dir =
            FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dir_name);
        if (!new_dir) {
          return false;
        }

        FilePtr file = shared_from_this();
        if (!old_dir->RemoveChildFile(file)) {
          return false;
        }
        if (!FileSystem::Instance()->file_handle()->RemoveFileNoLock(file)) [[unlikely]] {
          return false;
        }

        std::string file_name = GetFileName(to);
        set_file_name(file_name);
        set_dh(new_dir->dh());
        using namespace std::chrono;
        system_clock::time_point now = system_clock::now();
        time_t seconds = system_clock::to_time_t(now);
        UpdateTimeStamp(seconds);

        // 添加新的文件映射
        FileSystem::Instance()->file_handle()->AddFileNoLock(file);
        if (!new_dir->AddChildFile(file)) {
          return false;
        }

        if (!WriteMeta()) {
          return false;
        }
        return true;
      });
  return success ? 0 : -1;
}

const uint32_t File::GetBlockNumber() const noexcept {
  uint32_t num = 0;
  for (const auto &it : item_maps_) {
    const FileBlockPtr &fb = it.second;
    num += fb->used_block_num();
  }
  LOG(DEBUG) << file_name() << " has block number: " << num;
  return num;
}

void File::stat(struct stat *buf) {
  FileSystem::Instance()->file_handle()->RunInMetaGuard([this, buf] {
    uint64_t numer_of_block_512 = GetBlockNumber() * kBlockSize / 512;
    /* initialize all the values */
    buf->st_dev = 0;                     /* ID of device containing file */
    buf->st_ino = fh();                  /* inode number */
    buf->st_mode = 0755;                 /* protection */
    buf->st_nlink = nlink_;              /* number of hard links */
    buf->st_uid = ::getuid();            /* user ID of owner */
    buf->st_gid = ::getgid();            /* group ID of owner */
    buf->st_rdev = 0;                    /* device ID (if special file) */
    buf->st_blksize = kBlockSize; /* blocksize for file system I/O */
    buf->st_blocks = numer_of_block_512; /* number of 512B blocks allocated */
    buf->st_atime = atime();             /* time of last access */
    buf->st_mtime = mtime();             /* time of last modification */
    buf->st_ctime = ctime();             /* time of last status change */
    buf->st_size = meta_->size_;         /* total size, in bytes */
    buf->st_mode |= S_IFREG; /* specify whether item is a file or directory */
    return true;
  });
}

bool File::Release() {
  return FileSystem::Instance()->file_handle()->RunInMetaGuard(
      [this] { return ReleaseNolock(); });
}

bool File::ReleaseNolock() {
  // TODO:判断是否有子文件
  File::ClearMeta(meta_);
  LOG(INFO) << "release file name: " << file_name() << " fh: " << fh();
  return WriteMeta();
}

bool File::WriteMeta() { return File::WriteMeta(fh()); }

void File::DumpMeta() {
  LOG(INFO) << "dump file meta: "
            << " file_name: " << file_name() << " file_size: " << file_size()
            << " fh: " << fh() << " dh: " << dh()
            << " crc: " << meta_->crc_ << " parent fh: " << parent_fh()
            << " parent size: " << parent_size() << " child fh: " << child_fh();
  for (const auto &it : item_maps_) {
    const FileBlockPtr &fb = it.second;
    fb->DumpMeta();
  }
}

bool File::AddFileBlockLock(const FileBlockPtr &fb) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return AddFileBlockNoLock(fb);
}

bool File::AddFileBlockNoLock(const FileBlockPtr &fb) noexcept {
  auto it = item_maps_.find(fb->file_cut());
  if (it != item_maps_.end()) [[unlikely]] {
    SPDLOG_INFO("{} failed to add fb because already exist: {}", file_name(), fb->index());
    return false;
  }
  item_maps_[fb->file_cut()] = fb;
  LOG(TRACE) << "file block idx: " << fb->index()
             << " add to file: " << file_name()
             << " file block size: " << item_maps_.size()
             << " block id num: " << fb->used_block_num();
  return true;
}

bool File::RemoveAllFileBlock() {
  for (auto iter = item_maps_.begin(); iter != item_maps_.end(); ++iter) {
    FileBlockPtr fb = iter->second;
    if (!fb->ReleaseAll()) {
      return false;
    }
  }
  item_maps_.clear();
  // 清空列表
  assert(item_maps_.empty());
  return true;
}

FileBlockPtr File::GetFileBlock(int32_t file_cut) {
  SPDLOG_INFO("{} get file block cut: {} file block size: {}", file_name(), file_cut, item_maps_.size());
  auto iter = item_maps_.find(file_cut);
  if (iter == item_maps_.end()) [[unlikely]] {
    LOG(ERROR) << "cannot find file cut: " << file_cut;
    return nullptr;
  }
  return iter->second;
  // return GetMetaItem(file_cut);
}

int File::ExtendFile(uint64_t offset) {
  if (offset > FileSystem::Instance()->super_meta()->device_size) [[unlikely]] {
    LOG(ERROR) << file_name() << " truncate offset: " << offset
               << " cannot exceed free uidsk size: "
               << FileSystem::Instance()->super_meta()->device_size;
    return -1;
  }
  // 计算需要扩大偏移的位置
  uint64_t expand_size = offset - meta_->size_;
  SPDLOG_INFO("{} need expand size: {} offset: {} current size: {}", file_name(), expand_size, offset, file_size());

  // 1. Count last block space left
  uint64_t last_block_left;
  if (meta_->size_ % kBlockSize == 0) {
    last_block_left = 0;
  } else {
    // 去掉最后一个block的剩余空间
    last_block_left = kBlockSize - meta_->size_ % kBlockSize;
  }

  uint32_t num_blocks_needed;
  if (expand_size <= last_block_left) {
    num_blocks_needed = 0;
  } else {
    num_blocks_needed = ALIGN_UP((expand_size - last_block_left), kBlockSize);
  }
  LOG(INFO) << file_name() << " last block left: " << last_block_left
            << " need alloc block num: " << num_blocks_needed;

  // 是否需要申请fileblock
  FileBlockPtr file_block = nullptr;
  bool need_new_file_block = false;
  if (meta_->size_ == 0) {
    assert(item_maps_.size() == 0);
    file_block = FileSystem::Instance()->file_block_handle()->GetFileBlockLock();
    if (!file_block) {
      return -1;
    }
    need_new_file_block = true;
  } else {
    // 找到的最大的file cut的file block
    assert(item_maps_.size() > 0);
    file_block = GetFileBlock(item_maps_.size() - 1);
    if (nullptr == file_block) [[unlikely]] {
      return -1;
    }
    if (file_block->is_block_full()) {
      LOG(ERROR) << file_name() << " file block is full";
      return -1;
    }
    need_new_file_block = false;
  }

  std::vector<uint32_t> block_ids;
  if (num_blocks_needed > 0) {
    if (!FileSystem::Instance()->block_handle()->GetFreeBlockIdLock(
            num_blocks_needed, &block_ids)) {
      // 此处需要释放fileblock, 回退资源
      if (need_new_file_block) {
        FileSystem::Instance()->file_block_handle()->PutFileBlockLock(file_block);
      }
      return -1;
    }
  }
  if (need_new_file_block) {
    file_block->set_used(true);
    file_block->set_file_cut(0);
    file_block->set_fh(meta_->fh_);
    file_block->set_is_temp(this->is_temp());
  }

  // 开始填充fileblock的参数
  uint32_t block_ids_cursor = 0;
  // 存在新申请block的的场景
  while (block_ids_cursor < block_ids.size()) {
    if (file_block && !file_block->is_block_full()) {
      // 如果fileblock没有填满,并且blockid还没填充使用完
      SPDLOG_INFO("{} fill file block, used_block_num: {} block id: {}", file_name(), file_block->used_block_num(), block_ids[block_ids_cursor]);
      file_block->add_block_id(block_ids[block_ids_cursor++]);
      SPDLOG_INFO("{} fill file block, used_block_num: {} left block ids num: {}", file_name(), file_block->used_block_num(), (block_ids.size() - block_ids_cursor));
    } else {
      LOG(ERROR) << file_name() << " file block is full";
      return -1;
    }
  }

  // 开始持久化fileblock元数据
  // 新增了block才会涉及到fileblock的更新
  // 否则就是在原来的block的基础上更新下文件offset即可
  if (block_ids.size() > 0) {
    if (!file_block->WriteMeta()) {
      return -1;
    }
    if (need_new_file_block) {
      // 更新文件内存中fileblock信息
      AddFileBlockNoLock(file_block);
    }
  }

  bool success =
      FileSystem::Instance()->file_handle()->RunInMetaGuard([this, offset] {
        // 更新内存: 文件的大小
        set_file_size(offset);
        // 开始持久化更新文件元数据
        if (!WriteMeta()) {
          return false;
        }
        return true;
      });
  if (!success) [[unlikely]] {
    return -1;
  }
  return 0;
}

bool File::CopyData(uint32_t src_block_id, uint32_t dst_block_id,
                    uint64_t offset, uint64_t len) {
  const uint64_t kZeroBufferSize = 4 * M;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      kZeroBufferSize, FileSystem::Instance()->dev()->block_size());
  int64_t ret;
  uint64_t data_len = 0;
  uint64_t data_offset = 0;
  while (len > 0) {
    data_len = std::min(len, kZeroBufferSize);
    ::memset(buffer->data(), 0, data_len);
    data_offset =
        FileSystem::Instance()->super_meta()->block_data_start_offset_ +
        FileSystem::Instance()->super_meta()->block_size_ * src_block_id +
        offset;
    ret = FileSystem::Instance()->dev()->PreadCache(buffer->data(), data_len,
                                                   data_offset);
    if (ret != static_cast<int64_t>(data_len)) {
      LOG(ERROR) << file_name() << " read data error size: " << ret
                 << "data_len: " << data_len;
      return false;
    }
    data_offset =
        FileSystem::Instance()->super_meta()->block_data_start_offset_ +
        FileSystem::Instance()->super_meta()->block_size_ * dst_block_id +
        offset;
    ret = FileSystem::Instance()->dev()->PwriteCache(buffer->data(), data_len,
                                                    data_offset);
    if (ret != static_cast<int64_t>(data_len)) {
      LOG(ERROR) << file_name() << " write data error size: " << ret
                 << "data_len: " << data_len;
      return false;
    }
    len -= data_len;
    offset += data_len;
  }
  ret = FileSystem::Instance()->dev()->Fsync();
  SPDLOG_INFO("{} copy data success", file_name());
  return (ret == 0);
}

// 针对父fh进行回收
// 如果是单机场景直接回收
// 如果是主从场景需要确认
int File::RecycleParentFh(int32_t fh, bool immediately) {
  if (immediately) {
    LOG(INFO) << file_name() << " recycle parent fh immediately";
    if (!FileSystem::Instance()->file_handle()->RemoveParentFile(fh)) {
      return -1;
    }
    bool success = FileSystem::Instance()->file_handle()->RunInMetaGuard([this] {
      set_parent_fh(-1);
      set_parent_size(0);
      if (!WriteMeta()) {
        return false;
      }
      return true;
    });
    return success ? 0 : -1;
  }
  return 0;
}

// truncate变小的场景 需要新申请文件fh继承来自老的fh
int File::ShrinkFile(uint64_t offset) {
  // 先保证申请fh成功, 并且如果需要涉及到block的申请和拷贝
  // TODO: 可以先使用临时的meta保证不被写入到磁盘上去
  FileMeta *new_meta =
      FileSystem::Instance()->file_handle()->NewFreeFileMeta(dh(), file_name());
  if (!new_meta) [[unlikely]] {
    return -1;
  }
  FileMeta *old_meta = meta();

  uint32_t block_num = offset / kBlockSize;
  uint64_t block_offset = offset % kBlockSize;

  // uint32_t file_cut = block_num / kFileBlockCapacity;
  uint32_t file_cut = 0;
  uint32_t file_block_offset = block_num % kFileBlockCapacity;

  // 继承临时文件属性
  new_meta->is_temp_ = old_meta->is_temp_;
  new_meta->parent_fh_ = old_meta->fh_;
  new_meta->parent_size_ = block_num * kBlockSize;
  // 先赋值继承父文件的size,如果需要单独申请 一个block,做单独的申请
  new_meta->size_ = block_num * kBlockSize;

  // 新申请FileBlock把blockid拷贝过来
  // 开始申请资源
  uint32_t file_block_num = file_cut;
  // 多申请一个FileBlock
  // 有可能是为了有多个剩余的block
  // 有可能是为了放最后一个copy的block
  if (file_block_offset > 0 || block_offset > 0) {
    file_block_num = 1;
  }
  FileBlockPtr file_block = nullptr;
  if (file_block_num > 0) {
    file_block = FileSystem::Instance()->file_block_handle()->GetFileBlockLock();
    if (!file_block) {
      return -1;
    }
    // 拷贝block_num, 可能还需要单独申请一个
    uint32_t copy_block_num = 0;
    bool copy_compelted = false;
    FileBlockPtr old_fb = nullptr;
    FileBlockPtr new_fb = nullptr;
    for (uint32_t i = 0; i < file_block_num; ++i) {
      old_fb = GetMetaItem(i);
      new_fb = file_block;
      assert(old_fb->file_cut() == i);
      new_fb->set_used(true);
      new_fb->set_file_cut(old_fb->file_cut());
      new_fb->set_fh(new_meta->fh_);
      new_fb->set_is_temp(new_meta->is_temp_);
      if (block_num == 0) {
        LOG(INFO) << file_name()
                  << " no completed block inherited from parent file";
        break;
      }

      for (uint32_t j = 0; j < kFileBlockCapacity; ++j) {
        LOG(INFO) << file_name() << " now copy parent block index: " << i
                  << " block id: " << old_fb->meta()->block_id_[j];
        new_fb->add_block_id(old_fb->meta()->block_id_[j]);
        ++copy_block_num;
        if (copy_block_num == block_num) {
          LOG(INFO) << file_name() << " copy block id compelted";
          copy_compelted = true;
          break;
        }
      }
      if (copy_compelted) break;
    }
    // 在block中间截断需要单独申请一个block来承载
    if (block_offset > 0) {
      std::vector<uint32_t> block_ids;
      if (!FileSystem::Instance()->block_handle()->GetFreeBlockIdLock(
              1, &block_ids)) {
        return -1;
      }
      uint32_t new_block_id = block_ids[0];
      LOG(INFO) << file_name() << " new block id: " << new_block_id;
      LOG(INFO) << file_name() << " new file block id: " << new_fb->index();

      // 可能已经在下一个fileblock
      // 0 1 2 3 4 ............ 998 999 [1000个] 0 1 2 3 4 ......
      // x x x x x ............  x  [8M|8M]
      if (!CopyData(old_fb->meta()->block_id_[file_block_offset], new_block_id,
                    0, block_offset)) {
        return -1;
      }
      SPDLOG_INFO("{} file_block_num: {}", file_name(), file_block_num);
      // 填充到申请最后一个fileblock
      new_fb = file_block;
      new_fb->add_block_id(new_block_id);
    }
  }

  // 老的文件需要找到子文件
  old_meta->child_fh_ = new_meta->fh_;

  const DirectoryPtr &dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dh());
  if (!dir) [[unlikely]] {
    return -1;
  }
  // 删除之前的meta的fh映射
  FileSystem::Instance()->file_handle()->RemoveFileFromoDirectory(
      dir, shared_from_this());

  // 把文件的元数据更新到的FileMeta
  this->set_meta(new_meta);
  FileSystem::Instance()->file_handle()->AddFileToDirectory(dir,
                                                           shared_from_this());

  ParentFilePtr parent =
      ParentFile::NewParentFile(old_meta, offset, item_maps_);
  if (!parent || !FileSystem::Instance()->file_handle()->AddParentFile(parent)) {
    return -1;
  }

  if (file_block) {
    // 添加到文件内存的filecut映射中
    AddFileBlockNoLock(file_block);
  }

  // 更新到实际truncate后的size
  this->set_file_size(offset);

  // 先写新文件file元数据, 如果失败自然没有fileblock的元数据
  if (!WriteMeta()) {
    return -1;
  }
  // 写新文件FileBlock的元数据
  if (file_block) {
    if (!file_block->WriteMeta()) {
      return -1;
    }
  }

  // 新文件落盘, 再删除老文件
  if (!WriteMeta(old_meta->fh_)) {
    return -1;
  }

  LOG(INFO) << "shrink for: " << file_name()
            << " block_offset: " << block_offset;

  // 等从节点响应之后再回收父fh
  return RecycleParentFh(old_meta->fh_, true);
}

int File::ftruncate(uint64_t offset) {
  std::lock_guard<std::mutex> lock(mutex_);
  SPDLOG_INFO("file name: {} file size: {} ftruncate offset: {}", file_name(), file_size(), offset);
  if (offset > file_size()) {
    return ExtendFile(offset);
  } else if (offset < file_size()) {
    return ShrinkFile(offset);
  } else {
    LOG(WARNING) << file_name() << " truncate offset: " << offset
                 << " file size: " << file_size() << ", no need to truncate";
    return 0;
  }
}

int File::fsync() { return FileSystem::Instance()->dev()->Fsync(); }

void File::UpdateTimeStamp(time_t seconds) {
  std::lock_guard<std::mutex> lock(mutex_);

  set_mtime(seconds);
  set_ctime(seconds);

  meta_->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta_) + sizeof(meta_->crc_),
                      FileSystem::Instance()->super_meta()->file_meta_size_ -
                          sizeof(meta_->crc_));
}

bool File::UpdateMeta() { return WriteMeta(); }

/*****************************************************************/
// https://www.cnblogs.com/mickole/p/3182033.html
// 如果offset比文件的当前长度更大，下一个写操作就会把文件"撑大(extend)"
// 这就是所谓的在文件里创造"空洞(hole)"
// 没有被实际写入文件的所有字节由重复的0表示
// 空洞是否占用硬盘空间是由文件系统(file system)决定的.
off_t OpenFile::lseek(off_t offset, int whence) {
  /* get current file position */
  off_t current_pos = append_pos();

  SPDLOG_DEBUG("{} lseek fh: {} offset: {}", file_->file_name(), file_->fh(), current_pos);

  switch (whence) {
    case SEEK_SET:
      /* seek to offset */
      current_pos = offset;
      break;
    case SEEK_CUR:
      /* seek to current position + offset */
      current_pos += offset;
      break;
    case SEEK_END:
      /* seek to EOF + offset */
      current_pos = file_->file_size() + offset;
      break;
    default:
      errno = EINVAL;
      return (off_t)-1;
  }
  /* set and return final file position */
  // current_pos计算出来为负数如何处理?
  set_append_pos(current_pos);
  return current_pos;
}

int64_t OpenFile::read(void *buf, uint64_t size, uint64_t append_pos) {
  LOG(INFO) << "file name: " << file_->file_name()
            << " file size: " << file_->file_size() << " read size: " << size
            << " offset: " << append_pos;
  if (size == 0 || append_pos >= file_->file_size()) [[unlikely]] {
    LOG(ERROR) << file_->file_name() << " read nothing,"
                 << " read size: " << size << " read offset: " << append_pos
                 << " file size: " << file_->file_size();
    return 0;
  }
  uint64_t need_read_size = size;
  if (append_pos + size > file_->file_size()) {
    need_read_size = file_->file_size() - append_pos;
    SPDLOG_WARN("{} read file from offset to end, file size: {} offset: {}", file_->file_name(), file_->file_size(), append_pos);
  }

  FileReader reader =
      FileReader(shared_from_this(), buf, need_read_size, append_pos, false);
  int64_t ret = reader.ReadData();
  return ret;
}

int64_t OpenFile::FileReader::ReadBlockData(BlockData *block) {
  SPDLOG_INFO("{} read udisk offset: {} read size: {} buffer addr: 0x{}", open_file_->file()->file_name(), block->dev_offset, block->read_size_, (uint64_t)block->extern_buffer);
  if (direct_) {
    return FileSystem::Instance()->dev()->PreadDirect(
        block->extern_buffer, block->read_size_, block->dev_offset);
  } else {
    return FileSystem::Instance()->dev()->PreadCache(
        block->extern_buffer, block->read_size_, block->dev_offset);
  }
}

int64_t OpenFile::FileReader::ReadData() {
  // 按照block来切分递增计数直到和需要读取的size相等
  uint64_t curr_read_count = 0;
  uint32_t block_id = 0;   // 读取的是全局block的索引
  uint32_t block_read_offset = 0;  // 读取的block的偏移,初始为偏移为0
  uint64_t block_read_size = 0;    // 读取当前block的大小
  int32_t block_index_in_file_block = offset_ / kBlockSize;
  uint64_t block_offset_in_block = offset_ % kBlockSize;

  const FilePtr &file = open_file_->file();
  FileBlockPtr file_block = file->GetFileBlock(0);
  // 删除文件发生故障, 只删除了fileblock的情形
  if (nullptr == file_block) [[unlikely]] {
    return -1;
  }
  do {
    block_id = file_block->get_block_id(block_index_in_file_block);
    // 第一次填充的可能是在某个block中间的偏移
    if (block_offset_in_block > 0) {
      block_read_offset = block_offset_in_block;
      block_read_size = std::min(
          size_, (uint64_t)(kBlockSize - block_offset_in_block));
      block_offset_in_block = 0;
    } else {
      block_read_offset = 0;
      block_read_size =
          std::min(size_ - curr_read_count, (uint64_t)kBlockSize);
    }
    // 计算当前block在磁盘上的偏移
    uint64_t dev_offset =
        FileSystem::Instance()->super_meta()->block_data_start_offset_ +
        FileSystem::Instance()->super_meta()->block_size_ * block_id +
        block_read_offset;
    // TODO: 如果连续的block的话,读写可以考虑聚合优化
    uint64_t block_data_start_offset = FileSystem::Instance()->super_meta()->block_data_start_offset_;
    SPDLOG_INFO("{} data_start_offset: {} need read offset: {} block_id: {} block_read_offset: {} block_read_size: {} dev_offset: {}", file->file_name(), block_data_start_offset, curr_read_count, block_id, block_read_offset, block_read_size, dev_offset);
    BlockData block {
      .block_id = block_id,
      .extern_buffer = read_buffer_ + curr_read_count,
      .dev_offset = dev_offset,
      .read_size_ = block_read_size
    };
    read_blocks_.emplace_back(block);
    curr_read_count += block_read_size;
    if (curr_read_count >= size_) {
      SPDLOG_DEBUG("finshed fill read block");
      break;
    }
    // 按照block的粒度读取, 处理完一个block即block索引增加
    ++block_index_in_file_block;
  } while (true);

  int64_t ret = 0;
  int64_t block_read;
  uint32_t block_num = read_blocks_.size();
  for (uint32_t i = 0; i < block_num; ++i) {
    uint32_t block_id = read_blocks_[i].block_id;
    std::shared_lock lock(FileSystem::Instance()->block_handle()->block_lock(block_id));
    block_read = ReadBlockData(&read_blocks_[i]);
    if (block_read <= 0) [[unlikely]] {
      LOG(ERROR) << open_file_->file()->file_name()
                 << " read block data ret: " << block_read
                 << " errno: " << errno;
      break;
    }
    ret += block_read;
  }
  if (!direct_) {
    open_file_->set_append_pos(open_file_->append_pos() + ret);
  }
  return ret;
}

OpenFile::FileWriter::FileWriter(OpenFilePtr file, void *buffer, uint64_t size,
                                 uint64_t offset, bool direct)
    : open_file_(file),
      write_buffer_(static_cast<uint8_t *>(buffer)),
      size_(size),
      offset_(offset),
      direct_(direct) {
}

int64_t OpenFile::FileWriter::WriteBlockData(BlockData *block) {
  SPDLOG_INFO("{} write disk offset: {} write size: {} buffer addr: 0x{}", open_file_->file()->file_name(), block->dev_offset, block->write_size_, (uint64_t)block->extern_buffer);
  if (direct_) {
    return FileSystem::Instance()->dev()->PwriteDirect(
        block->extern_buffer, block->write_size_, block->dev_offset);
  } else {
    return FileSystem::Instance()->dev()->PwriteCache(
        block->extern_buffer, block->write_size_, block->dev_offset);
  }
}

int64_t OpenFile::FileWriter::WriteData() {
  int32_t block_index_in_file_block = offset_ / kBlockSize;
  uint64_t block_offset_in_block = offset_ % kBlockSize;

  uint64_t curr_write_count = 0;
  uint32_t block_id = 0;
  uint32_t block_write_offset = 0;
  uint64_t block_write_size = 0;

  const FilePtr &file = open_file_->file();
  FileBlockPtr file_block = file->GetFileBlock(0);
  do {
    block_id = file_block->get_block_id(block_index_in_file_block);

    //  第一次填充的可能是在某个block中间的偏移
    if (block_offset_in_block > 0) {
      block_write_offset = block_offset_in_block;
      block_write_size = std::min(
          size_, (uint64_t)(kBlockSize - block_offset_in_block));
      block_offset_in_block = 0;
    } else {
      block_write_offset = 0;
      block_write_size =
          std::min(size_ - curr_write_count, (uint64_t)kBlockSize);
    }
    uint64_t dev_offset =
        FileSystem::Instance()->super_meta()->block_data_start_offset_ +
        FileSystem::Instance()->super_meta()->block_size_ * block_id +
        block_write_offset;
    // TODO: 如果连续的block的话,读写可以考虑聚合优化
    uint64_t block_data_start_offset = FileSystem::Instance()->super_meta()->block_data_start_offset_;
    SPDLOG_INFO("{} data_start_offset: {} need wirte offset: {} block_id: {} block_write_offset: {} block_write_size: {} dev_offset: {}", file->file_name(), block_data_start_offset, curr_write_count, block_id, block_write_offset, block_write_size, dev_offset);
    BlockData block {
      .block_id = block_id,
      .extern_buffer = write_buffer_ + curr_write_count,
      .dev_offset = dev_offset,
      .write_size_ = block_write_size
    };
    write_blocks_.emplace_back(block);
    curr_write_count += block_write_size;
    if (curr_write_count >= size_) {
      SPDLOG_DEBUG("{} finshed fill write block", file->file_name());
      break;
    }
    ++block_index_in_file_block;
  } while (true);

  int64_t ret = 0;
  int64_t block_write;
  uint32_t block_num = write_blocks_.size();
  for (uint32_t i = 0; i < block_num; ++i) {
    uint32_t block_id = write_blocks_[i].block_id;
    std::unique_lock lock(FileSystem::Instance()->block_handle()->block_lock(block_id));
    block_write = WriteBlockData(&write_blocks_[i]);
    if (block_write <= 0) [[unlikely]] {
      LOG(ERROR) << open_file_->file()->file_name()
                 << " write block data ret: " << block_write
                 << " errno: " << errno;
      break;
    }
    ret += block_write;
  }
  if (!direct_) {
    open_file_->set_append_pos(open_file_->append_pos() + ret);
  }
  // size_
  return ret;
}

/* read count bytes info buf from file starting at offset pos,
 * returns number of bytes actually read in retcount,
 * retcount will be less than count only if an error occurs
 * or end of file is reached */
int64_t OpenFile::pread(void *buf, uint64_t size, uint64_t offset) {
  SPDLOG_INFO("file name: {} file size: {} pread size: {} offset: {}", file_->file_name(), file_->file_size(), size, offset);
  if (size == 0 || offset >= file_->file_size()) [[unlikely]] {
    LOG(WARNING) << file_->file_name() << " read nothing,"
                 << " read size: " << size << " read offset: " << offset
                 << " file size: " << file_->file_size();
    return 0;
  }
  uint64_t need_read_size = size;
  if (offset + size > file_->file_size()) {
    need_read_size = file_->file_size() - offset;
    LOG(WARNING) << file_->file_name()
                 << " read file from offset to end, file size:"
                 << file_->file_size() << " offset: " << offset;
  }

  FileReader reader =
      FileReader(shared_from_this(), buf, need_read_size, offset, false);
  int64_t ret = reader.ReadData();
  return ret;
}

/* write count bytes from buf into file starting at offset pos,
 * allocates new bytes and updates file size as necessary,
 * fills any gaps with zeros */
int64_t OpenFile::pwrite(const void *buf, uint64_t size, uint64_t offset) {
  SPDLOG_INFO("file name: {} file size: {} pwrite size: {} offset: {}", file_->file_name(), file_->file_size(), size, offset);
  void *buffer = const_cast<void *>(buf);
  if (size == 0) [[unlikely]] {
    return 0;
  }

  if ((offset + size) > file_->file_size()) {
    LOG(INFO) << file_->file_name() << " pwrite exceed file size,"
                 << " file size: " << file_->file_size()
                 << " write offset: " << offset << " write size: " << size;
    if (file_->ftruncate(offset + size) < 0) {
      return -1;
    }
  }

  // 减少时间戳更新的次数
  using namespace std::chrono;
  system_clock::time_point now = system_clock::now();
  time_t seconds = system_clock::to_time_t(now);
  if (seconds % 2 == 0) {
    file_->UpdateTimeStamp(seconds);
  }

  // 目前不能以direct方式打开, 因为如果以direct方式打开, 必须要扇区对齐写入
  FileWriter writer = FileWriter(shared_from_this(), buffer, size, offset, false);
  int64_t ret = writer.WriteData();
  return ret;
}

}