// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_H_
#define LIB_FILE_H_

#include <shared_mutex>

#include "block_device.h"
#include "inode.h"
#include "logging.h"

namespace udisk::blockfs {

class FileBlock;
typedef std::shared_ptr<FileBlock> FileBlockPtr;

typedef std::unordered_map<int32_t, FileBlockPtr> FileBlockMap;

class ParentFile;
typedef std::shared_ptr<ParentFile> ParentFilePtr;

class File;
typedef std::shared_ptr<File> FilePtr;

class OpenFile;
typedef std::shared_ptr<OpenFile> OpenFilePtr;

class ParentFile {
 private:
  FileMeta *meta_;
  uint64_t offset_;
  std::unordered_map<int32_t, FileBlockPtr> fb_maps_;
  bool tmp_file_ = false;

 public:
  static ParentFilePtr NewParentFile(FileMeta *meta, uint64_t offset,
                                     FileBlockMap &fbs, bool tmp_file = false);
  ParentFile(FileMeta *meta, uint64_t offset, FileBlockMap &fbs, bool tmp_file);
  ~ParentFile();
  int32_t fh() { return meta_->fh_; }
  bool tmp_file() const noexcept { return tmp_file_; }
  bool Recycle();
};

class File : public Inode<FileMeta, FileBlock>,
             public std::enable_shared_from_this<File> {
 private:
  std::atomic_int nlink_;

  bool locked_ = false;
  bool deleted_ = false;

 private:
  uint32_t GetNextFileCut() noexcept;
  int ExtendFile(uint64_t offset);
  bool CopyData(uint32_t src_block_id, uint32_t dst_block_id, uint64_t offset,
                uint64_t len);
  int RecycleParentFh(int32_t fh, bool immediately = false);
  int ShrinkFile(uint64_t offset);

 public:
  File();
  File(FileMeta *meta);
  virtual ~File();
  bool Release();
  bool ReleaseNolock();

  void IncLinkCount() noexcept { ++nlink_; }
  int32_t DecLinkCount() noexcept { return --nlink_; }
  int32_t LinkCount() const noexcept { return nlink_; }
  const bool is_temp() const noexcept { return meta_->is_temp_; }
  void set_is_temp(bool is_temp) noexcept { meta_->is_temp_ = is_temp; }
  void set_locked(bool locked) noexcept { locked_ = locked; }
  bool locked() const noexcept { return locked_; }
  void set_deleted(bool deleted) noexcept { deleted_ = deleted; }
  const bool deleted() const noexcept { return deleted_; }
  void set_crc(uint32_t crc) noexcept { meta_->crc_ = crc; }
  uint32_t crc() const noexcept { return meta_->crc_; }
  void set_used(bool used) const noexcept { meta_->used_ = used; }
  const bool used() const noexcept { return meta_->used_; }
  void set_fh(int32_t fh) noexcept { meta_->fh_ = fh; }
  int32_t fh() const noexcept { return meta_->fh_; }
  void set_dh(int32_t dh) noexcept { meta_->dh_ = dh; }
  int32_t dh() const noexcept { return meta_->dh_; }
  void set_file_size(uint64_t size) noexcept { meta_->size_ = size; }
  uint64_t file_size() const noexcept { return meta_->size_; }
  void set_file_name(const std::string &file_name) noexcept {
    ::memset(meta_->file_name_, 0, sizeof(meta_->file_name_));
    ::memcpy(meta_->file_name_, file_name.c_str(), sizeof(meta_->file_name_));
  }
  std::string file_name() const { return std::string(meta_->file_name_); }

  void set_child_fh(int32_t child_fh) noexcept { meta_->child_fh_ = child_fh; }
  int32_t child_fh() const noexcept { return meta_->child_fh_; }
  void set_parent_fh(int32_t parent_fh) noexcept {
    meta_->parent_fh_ = parent_fh;
  }
  int32_t parent_fh() const noexcept { return meta_->parent_fh_; }
  void set_parent_size(uint64_t parent_size) noexcept {
    meta_->parent_size_ = parent_size;
  }
  uint64_t parent_size() const noexcept { return meta_->parent_size_; }
  void set_atime(time_t time) noexcept override { meta_->atime_ = time; }
  time_t atime() noexcept override { return meta_->atime_; }
  void set_mtime(time_t time) noexcept override { meta_->mtime_ = time; }
  time_t mtime() noexcept override { return meta_->mtime_; }
  void set_ctime(time_t time) noexcept override { meta_->ctime_ = time; }
  time_t ctime() noexcept override { return meta_->ctime_; }
  const uint32_t GetBlockNumber() const noexcept;

  bool AddFileBlockLock(const FileBlockPtr &fb) noexcept;
  bool AddFileBlockNoLock(const FileBlockPtr &fb) noexcept;
  bool RemoveFileBlock(const FileBlockPtr &fb);
  bool RemoveAllFileBlock();
  FileBlockPtr GetFileBlock(int32_t file_cut);

  bool WriteMeta() override;
  void DumpMeta() override;
  static bool WriteMeta(int32_t fh);
  static void ClearMeta(FileMeta *meta);

 public:
  // common file posix functions
  void stat(struct stat *buf) override;
  int rename(const std::string &to) override;

  void UpdateTimeStamp(bool a = false, bool m = false, bool c = false) override;
  bool UpdateMeta() override;

  // open file functions
  int ftruncate(uint64_t offset);
  int posix_fallocate(uint64_t offset, uint64_t size);
  int fsync();

  template <typename... Args>
  int fcntl(int cmd, Args &&... args);
};
typedef std::shared_ptr<File> FilePtr;

class OpenFile : public std::enable_shared_from_this<OpenFile> {
 private:
  FilePtr file_;            /* current open file pointer */
  int32_t open_fd_ = -1;    /* current open file descriptor */
  mode_t open_mode_ = 0;    /* current open mode */
  int32_t open_flags_ = 0;  /* current open file flag */
  uint64_t append_pos_ = 0; /* current open file offset */
  std::shared_mutex rw_lock_;
 private:
  struct FileOffset {
    int32_t file_block_index = 0;          /* in which file cut */
    int32_t block_index_in_file_block = 0; /* in which block index */
    uint64_t block_offset_in_block = 0;    /* offset located in final block */

    // Find the read and write offset information
    // according to the file read and write position
    FileOffset(const int64_t offset) {
      // 1. Count how many blocks are in front of the file
      int32_t num_blocks_front = offset / kBlockSize;

      // 2. Count in which file cut
      file_block_index = num_blocks_front / kFileBlockCapacity;

      // 3. Count in which block index
      block_index_in_file_block = num_blocks_front % kFileBlockCapacity;

      // 4. Count block offset in final block
      block_offset_in_block = offset % kBlockSize;

      LOG(INFO) << "current offset: " << offset
                << " file_block_index: " << file_block_index
                << " block_index_in_file_block: " << block_index_in_file_block
                << " block_offset_in_block: " << block_offset_in_block;
    }
  };
  // Transform info of block read or write
  struct BlockData {
    uint8_t *extern_buffer_;  // 读或者写buffer的地址,如果超过block会转换
    uint64_t udisk_offset_;  // 写入block的在udisk逻辑盘上的偏移地址
    union {
      uint64_t read_size_;
      uint64_t write_size_;
    };
  };
  class FileReader {
   private:
    OpenFilePtr open_file_; /* file object */
    uint8_t *read_buffer_;  /* user buffer holding data */
    uint64_t size_;         /* number of bytes to read */
    uint64_t offset_;       /* position within file to read from */
    bool direct_ = true;    /* whether using direct fd */

   private:
    std::vector<BlockData> read_blocks_;
    // Transform to read discrete block data
    void Transform2Block();
    int64_t ReadBlockData(BlockData *block);

   public:
    FileReader(OpenFilePtr file, void *buffer, uint64_t size, uint64_t offset,
               bool direct = true)
        : open_file_(file),
          read_buffer_(static_cast<uint8_t *>(buffer)),
          size_(size),
          offset_(offset),
          direct_(direct) {
      Transform2Block();
    }
    ~FileReader() = default;
    int64_t ReadData();
  };
  class FileWriter {
   private:
    OpenFilePtr open_file_; /* file object */
    uint8_t *write_buffer_; /* user buffer holding data */
    uint64_t size_;         /* number of bytes to write */
    uint64_t offset_;       /* position within file to write to */
    bool direct_ = true;    /* whether using direct fd */

   private:
    std::vector<BlockData> write_blocks_;
    // Transform to read discrete block data
    void Transform2Block();
    int64_t WriteBlockData(BlockData *block);

   public:
    FileWriter(OpenFilePtr file, void *buffer, uint64_t size, uint64_t offset,
               bool direct = true);
    ~FileWriter();
    int64_t WriteData();
  };

 public:
  OpenFile(const FilePtr &file) : file_(file) {}
  ~OpenFile() = default;
  void set_open_flags(int32_t flag) noexcept { open_flags_ |= flag; }
  int32_t open_flags() const noexcept { return open_flags_; }
  uint64_t append_pos() const noexcept { return append_pos_; }
  void set_append_pos(uint64_t offset) noexcept { append_pos_ = offset; }

  const FilePtr &file() noexcept { return file_; }

  off_t lseek(off_t offset, int whence);

  int64_t read(void *buf, uint64_t size, uint64_t append_pos);
  int64_t write(const void *buf, uint64_t size, uint64_t append_pos);
  int64_t pread(void *buf, uint64_t size, uint64_t offset);
  int64_t pwrite(const void *buf, uint64_t size, uint64_t offset);

 private:
  friend class File;
};

}
#endif