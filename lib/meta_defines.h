#ifndef LIB_META_DEFINES_H
#define LIB_META_DEFINES_H

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace udisk::blockfs {

inline void block_fs_set_errno(int e) { errno = e; }

struct block_fs_dirent {
  long d_ino;                /* inode number */
  off_t d_off;               /* offset to this dirent */
  uint32_t d_reclen;         /* length of this d_name */
  uint32_t d_type;           /* the type of d_name */
  uint32_t d_link;           /* link conut */
  char d_name[NAME_MAX + 1]; /* file name (null-terminated) */
  time_t d_time_;
};

constexpr uint64_t K = 1024;
constexpr uint64_t M = (1024 * K);
constexpr uint64_t G = (1024 * M);
constexpr uint64_t T = (1024 * G);

constexpr uint32_t kBlockFsMagic = 0xA5201314;

constexpr uint64_t kUUIDSize = 64;
constexpr uint64_t kUXDBMountPrefixMaxLen = 256;

constexpr uint64_t kBlockFsMaxUxdbPrefixDirLen = (2 * K);

constexpr uint64_t kBlockFsPageSize = 4 * K;
constexpr uint64_t kSuperBlockSize = kBlockFsPageSize;
constexpr uint64_t kSuperBlockOffset = 0;
constexpr uint64_t kDirMetaOffset = (kSuperBlockOffset + kSuperBlockSize);

constexpr uint64_t kBlockFsMaxFileNum = 100000;
constexpr uint64_t kBlockFsMaxDirNum = kBlockFsMaxFileNum;
constexpr uint64_t kBlockFsMaxUDiskSize = (128 * T);
constexpr uint64_t kBlockSize = 4 * M;
constexpr uint64_t kBlockFsMaxDirNameLen = 768;
constexpr uint64_t kBlockFsMaxFileNameLen = 64;
constexpr uint64_t kBlockFsDirMetaSize = (1 * K);
constexpr uint64_t kBlockFsFileMetaSize = 256;
constexpr uint64_t kFileBlockCapacity = 1000;
constexpr uint64_t kBlockFsFileBlockMetaSize = kBlockFsPageSize;
constexpr uint64_t kBlockFsFileMetaIndexSize = kBlockFsPageSize;

typedef uint64_t ino_t;
typedef int32_t dh_t;
// Represents a sequence number in a WAL file.
typedef uint64_t seq_t;

static const seq_t kReservedUnusedSeq = 0;  // 0 is reserved sequence
static const seq_t kMinUnCommittedSeq = 1;  // 1 is always committed
static const seq_t kMaxUnCommittedSeq = UINT64_MAX;

/* 文件系统的超级块
 * 大小: 4K
 * 作用: 记录文件系统的一些规格参数
 * 操作: 提前利用部署工具写入参数
 **/
union SuperBlockMeta {
  struct {
    uint32_t crc_;
    char uuid_[kUUIDSize];  // mfs uuid -> /dev/vdc
    uint32_t magic_;                  // bfs magic
    seq_t seq_no_;                    // bfs sequenece
    // mfs consts
    uint64_t max_file_num;             // max supported dirs or files 10w
    uint64_t max_support_udisk_size_;  // max supported udisk size 128T
    uint64_t max_file_block_num;       // max supported file block(128T)
    uint64_t max_support_block_num_;   // max supported block num(128T)
    uint64_t block_size_;              // 4M, mfs block size
    uint64_t max_dir_name_len_;        // 64B, max dir name length
    uint64_t max_file_name_len_;       // 64B, max file name length
    uint64_t dir_meta_size_;           // 1K, per directory meta size
    uint64_t file_meta_size_;          // 256B, per file meta size
    uint64_t file_block_meta_size_;    // 4k, per file block meta size

    // The total size of each bfs metadata
    uint64_t super_block_size_;            // total size of super block
    uint64_t dir_meta_total_size_;         // total size of directory meta
    uint64_t file_meta_total_size_;        // total size of file meta
    uint64_t file_block_meta_total_size_;  // total size of file block meta

    // The offset start in udisk device offset
    uint64_t super_block_offset_;       // the offset of supber block
    uint64_t dir_meta_offset_;          // the offset of directory meta
    uint64_t file_meta_offset_;         // the offset of file meta
    uint64_t file_block_meta_offset_;   // the offset of file block meta
    uint64_t block_data_start_offset_;  // the offset of data block

    // variable according to the udisk device size
    uint64_t device_size;  // current udisk size (device size)
    uint64_t curr_block_num;   // current udisk supported block number

    // The mount point opened by each instance of UXDB,
    // the directory is generally /mnt/mysql/data
    char uxdb_mount_point_[kUXDBMountPrefixMaxLen];
  } __attribute__((packed));
  char reserved_[kSuperBlockSize];
};

static_assert(sizeof(SuperBlockMeta) == kSuperBlockSize,
              "SuperBlockMeta size must be 4096 Bytes");

union DirMeta {
  struct {
    uint32_t crc_;
    dh_t dh_;
    seq_t seq_no_;
    uint64_t size_;
    mode_t mode_;
    uid_t uid_;     // user ID of owner
    gid_t gid_;     // group ID of owner
    time_t atime_;  // 最后一次访问时间
    time_t mtime_;  // 最后一次修改时间
    time_t ctime_;  // 最后一次改变时间
    bool used_;
    // 包含多级目录,存成扁平的一级目录 512 + 256
    char dir_name_[kBlockFsMaxDirNameLen];
  } __attribute__((packed));
  char reserved_[kBlockFsDirMetaSize];
};

static_assert(sizeof(DirMeta) == kBlockFsDirMetaSize,
              "DirMeta size must be 1024 Bytes");

union FileMeta {
  struct {
    // bool deleted_;
    uint32_t crc_;
    int32_t fh_;
    seq_t seq_no_;
    uint64_t size_;
    int32_t nlink_;
    mode_t mode_;
    uid_t uid_;     // user ID of owner
    gid_t gid_;     // group ID of owner
    time_t atime_;  // last access times
    time_t mtime_;  // last modify times
    time_t ctime_;  // last change times
    dh_t dh_;       // belong to one dir
    int32_t child_fh_;
    int32_t parent_fh_;
    uint64_t parent_size_;
    bool used_;
    bool is_temp_;
    char file_name_[kBlockFsMaxFileNameLen];
  } __attribute__((packed));
  char reserved_[kBlockFsFileMetaSize];
};

static_assert(sizeof(FileMeta) == kBlockFsFileMetaSize,
              "FileMeta size must be 256 Bytes");

union FileBlockMeta {
  struct {
    uint32_t crc_;
    uint32_t index_;
    seq_t seq_no_;
    uint32_t fh_;
    uint32_t file_cut_;
    uint32_t padding_;
    uint32_t used_block_num_;
    bool used_;
    bool is_temp_;
    uint32_t block_id_[kFileBlockCapacity];
  } __attribute__((packed));
  char reserved_[kBlockFsFileBlockMetaSize];

  // 重载<运算符
  bool operator<(const FileBlockMeta &right) const {
    return file_cut_ < right.file_cut_;
  }
};

static_assert(sizeof(FileBlockMeta) == kBlockFsFileBlockMetaSize,
              "BlockFsBlockMeta size must be 4096 Bytes");

}  // namespace udisk::blockfs
#endif