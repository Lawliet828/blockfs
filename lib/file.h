// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_FILE_H_
#define LIB_FILE_H_

#include <shared_mutex>

#include "block_device.h"
#include "inode.h"

namespace udisk {
namespace blockfs {

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

  // File read/write lock, write first
  std::shared_mutex io_lock_;

 private:
  enum FileType {
    FILE_TYPE_NONE,       // None (file not found)
    FILE_TYPE_REGULAR,    // Regular file
    FILE_TYPE_DIRECTORY,  // Directory
    FILE_TYPE_SYMLINK,    // Symbolic link
    FILE_TYPE_BLOCK,      // Block device
    FILE_TYPE_CHARACTER,  // Character device
    FILE_TYPE_FIFO,       // FIFO (named pipe)
    FILE_TYPE_SOCKET,     // Socket
    FILE_TYPE_UNKNOWN     // Unknown
  };
  // File permissions (Unix specific)
  enum FilePermissions {
    NONE = 00000,   //!< None
    IRUSR = 00400,  //!< Read permission bit for the owner of the file
    IWUSR = 00200,  //!< Write permission bit for the owner of the file
    IXUSR = 00100,  //!< Execute (for ordinary files) or search (for
                    //!< directories) permission bit for the owner of the file
    IRWXU = 00700,  //!< This is equivalent to IRUSR | IWUSR | IXUSR
    IRGRP = 00040,  //!< Read permission bit for the group owner of the file
    IWGRP = 00020,  //!< Write permission bit for the group owner of the file
    IXGRP = 00010,  //!< Execute or search permission bit for the group owner of
                    //!< the file
    IRWXG = 00070,  //!< This is equivalent to IRGRP | IWGRP | IXGRP
    IROTH = 00004,  //!< Read permission bit for other users
    IWOTH = 00002,  //!< Write permission bit for other users
    IXOTH = 00001,  //!< Execute or search permission bit for other users
    IRWXO = 00007,  //!< This is equivalent to IROTH | IWOTH | IXOTH
    ISUID = 04000,  //!< This is the set-user-ID on execute bit
    ISGID = 02000,  //!< This is the set-group-ID on execute bit
    ISVTX = 01000   //!< This is the sticky bit
  };

 private:
  uint32_t GetNextFileCut() noexcept;
  int ExtendFile(uint64_t offset);
  bool CopyData(uint32_t src_block_id, uint32_t dst_block_id, uint64_t offset,
                uint64_t len);
  void CopyOldFileBlock();
  int RecycleParentFh(int32_t fh, bool immediately = false);
  int ShrinkFile(uint64_t offset);

  void AddRef();
  void DecRef();

 public:
  File();
  File(FileMeta *meta);
  virtual ~File();
  bool Create(int32_t dh, const std::string &filename);
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
  const uint64_t seq_no() const { return meta_->seq_no_; }
  void set_seq_no(uint64_t seq_no) const noexcept { meta_->seq_no_ = seq_no; }
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

  void WriteLockAcquire();
  void WriteLockRelease();
  void ReadLockAcquire();
  void ReadLockRelease();

 public:
  // common file posix functions
  void stat(struct stat *buf) override;
  int chmod(mode_t mode) override { return 0; }
  int chown(uid_t uid, gid_t gid) override { return 0; }
  int access(int mask) const override { return 0; }
  int rename(const std::string &to) override;
  int utimens(const timespec lastAccessTime,
              const timespec lastModificationTime) override {
    return 0;
  }
  int remove() override { return 0; }

  void UpdateTimeStamp(bool a = false, bool m = false, bool c = false) override;
  bool UpdateMeta() override;

  // open file functions
  int ftruncate(uint64_t offset);
  int posix_fallocate(uint64_t offset, uint64_t size);
  int fsync();
  int fdatasync();
  int64_t sendfile(int32_t out_fd, uint64_t *offset, uint64_t count);

  template <typename... Args>
  int fcntl(int cmd, Args &&... args);
};

class OpenFile : public std::enable_shared_from_this<OpenFile> {
 private:
  FilePtr file_;            /* current open file pointer */
  int32_t open_fd_ = -1;    /* current open file descriptor */
  mode_t open_mode_ = 0;    /* current open mode */
  int32_t open_flags_ = 0;  /* current open file flag */
  uint64_t append_pos_ = 0; /* current open file offset */
 private:
  struct FileOffset {
    int32_t file_block_index_ = 0;          /* in which file cut */
    int32_t block_index_in_file_block_ = 0; /* in which block index */
    uint64_t block_offset_in_block_ = 0;    /* offset located in final block */

    const int32_t file_block_index() const noexcept {
      return file_block_index_;
    }
    const int32_t block_index_in_file_block() const noexcept {
      return block_index_in_file_block_;
    }
    const uint64_t block_offset_in_block() const noexcept {
      return block_offset_in_block_;
    }
    FileOffset(const int64_t offset);
  };
  // Transdorm info of block read or write
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
               bool direct = true);
    ~FileReader();
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
  bool CheckIOParam(void *buffer, uint64_t size, uint64_t offset);
  class ExtendFileOffset {
   public:
    ExtendFileOffset() = default;
    ~ExtendFileOffset() = default;

   private:
  };

 public:
  OpenFile(const FilePtr &file) : file_(file) {}
  ~OpenFile() {}
  void set_open_flags(int32_t flag) noexcept { open_flags_ |= flag; }
  int32_t open_flags() const noexcept { return open_flags_; }
  uint64_t append_pos() const noexcept { return append_pos_; }
  void set_append_pos() noexcept { append_pos_ = file_->file_size(); }
  void set_append_pos(uint64_t offset) noexcept { append_pos_ = offset; }
  int32_t open_fd() const noexcept { return open_fd_; }
  void set_open_fd(int32_t fd) noexcept { open_fd_ = fd; }

  const FilePtr &file() noexcept { return file_; }

  off_t lseek(off_t offset, int whence);

  int64_t read(void *buf, uint64_t size, uint64_t append_pos);
  int64_t write(const void *buf, uint64_t size, uint64_t append_pos);
  int64_t pread(void *buf, uint64_t size, uint64_t offset);
  int64_t pwrite(const void *buf, uint64_t size, uint64_t offset);

 private:
  friend class File;
};

}  // namespace blockfs
}  // namespace udisk
#endif