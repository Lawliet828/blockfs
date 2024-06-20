#ifndef LIB_FILE_H_
#define LIB_FILE_H_

#include <atomic>
#include <shared_mutex>
#include <vector>

#include "device.h"
#include "inode.h"
#include "logging.h"

namespace udisk::blockfs {

class FileBlock;
typedef std::shared_ptr<FileBlock> FileBlockPtr;

typedef std::unordered_map<int32_t, FileBlockPtr> FileBlockMap;

class OpenFile;
typedef std::shared_ptr<OpenFile> OpenFilePtr;

class ParentFile {
 private:
  FileMeta *meta_;
  uint64_t offset_;
  std::unordered_map<int32_t, FileBlockPtr> fb_maps_;
  bool tmp_file_ = false;

 public:
  static std::shared_ptr<ParentFile> NewParentFile(FileMeta *meta, uint64_t offset,
                                     FileBlockMap &fbs, bool tmp_file = false);
  ParentFile(FileMeta *meta, uint64_t offset, FileBlockMap &fbs, bool tmp_file);
  ~ParentFile();
  int32_t fh() { return meta_->fh_; }
  bool tmp_file() const noexcept { return tmp_file_; }
  bool Recycle();
};
typedef std::shared_ptr<ParentFile> ParentFilePtr;

class File : public Inode<FileMeta, FileBlock>,
             public std::enable_shared_from_this<File> {
 private:
  std::atomic_int nlink_;

  bool locked_ = false;
  bool deleted_ = false;

 private:
  int ExtendFile(uint64_t offset);
  bool CopyData(uint32_t src_block_id, uint32_t dst_block_id, uint64_t offset,
                uint64_t len);
  int RecycleParentFh(int32_t fh, bool immediately = false);
  int ShrinkFile(uint64_t offset);

 public:
  File(): nlink_(0) {}
  File(FileMeta *meta): Inode(meta), nlink_(0) {}
  virtual ~File() = default;
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

  void UpdateTimeStamp(time_t seconds);
  bool UpdateMeta() override;

  // open file functions
  int ftruncate(uint64_t offset);
  int fsync();
};
typedef std::shared_ptr<File> FilePtr;

class OpenFile : public std::enable_shared_from_this<OpenFile> {
 private:
  FilePtr file_;            /* current open file pointer */
  int32_t open_fd_ = -1;    /* current open file descriptor */
  uint64_t append_pos_ = 0; /* current open file offset */
 private:
  // Transform info of block read or write
  struct BlockData {
    uint32_t block_id;
    uint8_t *extern_buffer;  // 读或者写buffer的地址,如果超过block会转换
    uint64_t dev_offset;  // 写入block的在udisk逻辑盘上的偏移地址
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
    int64_t ReadBlockData(BlockData *block);

   public:
    FileReader(OpenFilePtr file, void *buffer, uint64_t size, uint64_t offset,
               bool direct = true)
        : open_file_(file),
          read_buffer_(static_cast<uint8_t *>(buffer)),
          size_(size),
          offset_(offset),
          direct_(direct) {
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
    int64_t WriteBlockData(BlockData *block);

   public:
    FileWriter(OpenFilePtr file, void *buffer, uint64_t size, uint64_t offset,
               bool direct = true);
    ~FileWriter() = default;
    int64_t WriteData();
  };

 public:
  OpenFile(const FilePtr &file) : file_(file) {}
  ~OpenFile() = default;
  uint64_t append_pos() const noexcept { return append_pos_; }
  void set_append_pos(uint64_t offset) noexcept { append_pos_ = offset; }

  const FilePtr &file() noexcept { return file_; }

  off_t lseek(off_t offset, int whence);

  int64_t read(void *buf, uint64_t size, uint64_t append_pos);
  int64_t pread(void *buf, uint64_t size, uint64_t offset);
  int64_t pwrite(const void *buf, uint64_t size, uint64_t offset);

 private:
  friend class File;
};

}
#endif