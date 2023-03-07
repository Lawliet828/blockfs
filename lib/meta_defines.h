// Copyright (c) 2020 UCloud All rights reserved.
#ifndef LIB_META_DEFINES_H
#define LIB_META_DEFINES_H

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

#include "meta_consts.h"

namespace udisk {
namespace blockfs {

typedef int32_t dh_t;
// Represents a sequence number in a WAL file.
typedef uint64_t seq_t;

static const seq_t kReservedUnusedSeq = 0;  // 0 is reserved sequence
static const seq_t kMinUnCommittedSeq = 1;  // 1 is always committed
static const seq_t kMaxUnCommittedSeq = UINT64_MAX;

static const int32_t kJournalUnusedIndex = -1;
static const int32_t kMinJournalIndex = 0;
static const int32_t kMaxJournalIndex = 4095;

/* 协商区元数据
 * 大小: 4K
 * 作用: 比如UDisk逻辑盘扩容/缩容多少个Extent(10G)
 **/
enum ResizeType { ExpandExtend, ShrinkExtend };
union NegotMeta {
  struct {
    uint32_t crc_;
    seq_t seq_no_;
    ResizeType resize_type_;
    uint32_t resize_extend_num_;
  } __attribute__((packed));
  char reserved_[kNegotiationSize];
};

static_assert(sizeof(NegotMeta) == kNegotiationSize,
              "NegotMeta size must be 4096 Bytes");

/* 文件系统的超级块
 * 大小: 4K
 * 作用: 记录文件系统的一些规格参数
 * 操作: 提前利用部署工具写入参数
 **/
union SuperBlockMeta {
  struct {
    uint32_t crc_;
    char uuid_[kBlockFsMaxUuidSize];  // bfs uuid -> /dev/vdb
    uint32_t version_;                // bfs version
    uint32_t magic_;                  // bfs magic
    seq_t seq_no_;                 // bfs sequenece
    // bfs consts
    uint64_t max_support_file_num_;        // max supported dirs or files 10w
    uint64_t max_support_udisk_size_;      // max supported udisk size 128T
    uint64_t max_support_file_block_num_;  // max supported file block(128T)
    uint64_t max_support_block_num_;       // max supported block num(128T)
    uint64_t udisk_extend_size_;           // 10G, resize udisk size min unit
    uint64_t block_size_;                  // 16M, bfs min block size
    uint64_t max_dir_name_len_;            // 64B, max dir name length
    uint64_t max_file_name_len_;           // 64B, max file name length
    uint64_t dir_meta_size_;               // 1K, per directory meta size
    uint64_t file_meta_size_;              // 256B, per file meta size
    uint64_t file_block_meta_size_;        // 4k, per file block meta size
    uint64_t block_fs_journal_size_;       // 4K, per journal size
    uint64_t block_fs_journal_num_;        // total journal number

    // The total size of each bfs metadata
    uint64_t negotiation_size_;            // total size of negotiation
    uint64_t super_block_size_;            // total size of super block
    uint64_t dir_meta_total_size_;         // total size of directory meta
    uint64_t file_meta_total_size_;        // total size of file meta
    uint64_t file_block_meta_total_size_;  // total size of file block meta
    uint64_t journal_total_size_;          // total size of bfs journal

    // The offset start in udisk device offset
    uint64_t negotiation_offset_;       // the offset of negotiation
    uint64_t super_block_offset_;       // the offset of supber block
    uint64_t dir_meta_offset_;          // the offset of directory meta
    uint64_t file_meta_offset_;         // the offset of file meta
    uint64_t file_block_meta_offset_;   // the offset of file block meta
    uint64_t block_fs_journal_offset_;  // the offset of journal meta
    uint64_t block_data_start_offset_;  // the offset of data block

    // variable according to the udisk device size
    uint64_t curr_udisk_size_;  // current udisk size (device size)
    uint64_t curr_block_num_;   // current udisk supported block number
    uint64_t available_udisk_size_; // 可用空间大小

    // 待定
    // uint64_t all_dh_bitmap_;        // 所有已创建的文件夹
    // uint64_t all_fh_bitmap_;        // 所有已创建的文件
    // uint64_t head_;
    // uint64_t tail_;
    // uint64_t applied_seq_no_;

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
    dh_t dh_;    // belong to one dir
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
    uint32_t block_id_[kBlockFsFileBlockCapacity];
  } __attribute__((packed));
  char reserved_[kBlockFsFileBlockMetaSize];

  // 重载<运算符
  bool operator<(const FileBlockMeta &right) const {
    return file_cut_ < right.file_cut_;
  }
};

static_assert(sizeof(FileBlockMeta) == kBlockFsFileBlockMetaSize,
              "BlockFsBlockMeta size must be 4096 Bytes");

enum BlockFsJournalType {
  BLOCKFS_JOURNAL_CREATE_FILE,    // 创建文件
  BLOCKFS_JOURNAL_EXPAND_FILE,    // 文件扩容
  BLOCKFS_JOURNAL_TRUNCATE_FILE,  // 文件空洞
  BLOCKFS_JOURNAL_DELETE_FILE,    // 删除文件
  BLOCKFS_JOURNAL_RENAME_FILE,    // 文件重命名
  BLOCKFS_JOURNAL_CHMOD_FILE,     // 文件权限
  BLOCKFS_JOURNAL_CREATE_DIR,     // 创建文件夹
  BLOCKFS_JOURNAL_DELETE_DIR,     // 删除文件夹
  BLOCKFS_JOURNAL_CHMOD_DIR       // 文件夹权限
};

enum JOURNAL_OP {
  JOURNAL_OP_NONE = 0,   ///< none
  JOURNAL_OP_INIT,       ///< initial (empty) file system marker
  JOURNAL_OP_ALLOC_ADD,  ///< add extent to available block storage (extent)
  JOURNAL_OP_ALLOC_RM,  ///< remove extent from available block storage (extent)
  JOURNAL_OP_DIR_LINK,  ///< (re)set a dir entry (dirname, filename, ino)
  JOURNAL_OP_DIR_UNLINK,   ///< remove a dir entry (dirname, filename)
  JOURNAL_OP_DIR_CREATE,   ///< create a dir (dirname)
  JOURNAL_OP_DIR_REMOVE,   ///< remove a dir (dirname)
  JOURNAL_OP_DIR_UPDATE,   ///< set/update file metadata (file)
  JOURNAL_OP_FILE_CREATE,  ///< create a file (dirname)
  JOURNAL_OP_FILE_UPDATE,  ///< set/update file metadata (file)
  JOURNAL_OP_FILE_EXPAND,  ///< set/update file metadata (file)
  JOURNAL_OP_FILE_SHRINK,  ///< set/update file metadata (file)
  JOURNAL_OP_FILE_REMOVE,  ///< remove file (ino)
  JOURNAL_OP_JUMP,         ///< jump the seq # and offset
  JOURNAL_OP_JUMP_SEQ,     ///< jump the seq #
};

struct BlockFsJournalCreateFile {
  FileMeta file_meta_;
} __attribute__((packed));

struct BlockFsJournalCreateDir {
  DirMeta dir_meta_;
} __attribute__((packed));

struct BlockFsJournalExpandFile {
  int32_t fh_;
  uint64_t size_;             // 文件原来的大小
  uint32_t last_file_block_index_;
  uint32_t used_block_num_;   // 原文件最后一个file block原先使用的block数
  uint32_t file_block_num_;
  uint32_t file_block_ids_[1000];
} __attribute__((packed));

struct BlockFsJournalTruncateFile {
  FileMeta parent_file_meta_;
  int32_t new_fh_;
} __attribute__((packed));

struct BlockFsJournalDeleteFile {
  int32_t fh_;  // 文件句柄
} __attribute__((packed));

struct BlockFsJournalDeleteDir {
  dh_t dh_;  // 文件夹句柄
} __attribute__((packed));

struct BlockFsJournalRenameFile {
  int32_t fh_;                             // 原文件的句柄
  char file_name_[kBlockFsMaxFileNameLen]; // 新文件名
  dh_t dh_;                             // 新文件的目录
  time_t atime_;
  time_t mtime_;
  time_t ctime_;
} __attribute__((packed));

struct BlockFsJournalMoveFile {
  int32_t old_fh_;            // 文件句柄
  uint8_t target_file_[128];  // 目标文件的路径:rename, move操作
} __attribute__((packed));

// https://www.cnblogs.com/jikexianfeng/p/5742887.html
struct BlockFsJournalChmodFile {
  int32_t fh_;   // 文件句柄
  mode_t mode_;  // 文件权限
} __attribute__((packed));

struct BlockFsJournalChmodDir {
  dh_t dh_;   // 文件夹句柄
  mode_t mode_;  // 文件夹权限
} __attribute__((packed));

// BlockFsJournal 限制是每个4K
// 一个BlockFsJournal没办法容纳信息,可能需要分多个journal?
// 主需要告诉从分配的block相关元数据信息
// 多线程同一个fh最好落到一个个线程中 fh % thread
union BlockFsJournal {
  struct {
    uint32_t crc_;
    seq_t seq_no_;          // 操作序列号
    uint64_t jh_;              // journal handle编号
    BlockFsJournalType type_;  // 文件操作类型
    union {
      BlockFsJournalCreateFile create_file_;
      BlockFsJournalExpandFile expand_file_;
      BlockFsJournalTruncateFile truncate_file_;
      BlockFsJournalDeleteFile delete_file_;
      BlockFsJournalRenameFile rename_file_;
      BlockFsJournalMoveFile move_file_;
      BlockFsJournalChmodFile chmod_file_;
      BlockFsJournalCreateDir create_dir_;
      BlockFsJournalDeleteDir delete_dir_;
      BlockFsJournalChmodDir chmod_dir_;
    };
  } __attribute__((packed));
  uint8_t reserved_[kBlockFsJournalSize];
};

static_assert(sizeof(BlockFsJournal) == kBlockFsJournalSize,
              "BlockFsJournal size must be 4096 Bytes");

// 实现Journal的环形队列
struct BlockFsJournalRingBuffer {
  uint32_t head_;
  uint32_t tail_;
  BlockFsJournal journals_[0];  // 默认1024,可拓展
} __attribute__((packed));

// 主给从推送消息
struct BlockFsJournalMsg {
  seq_t seq_no_;
  int head_;
  int tail_;
} __attribute__((packed));
}  // namespace blockfs
}  // namespace udisk
#endif