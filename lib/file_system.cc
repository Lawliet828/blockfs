#include "file_system.h"

#include "comm_utils.h"
#include "logging.h"

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

/* 两次转换将宏的值转成字符串 */
#define _BFS_MACROSTR(a) (#a)
#define BFS_MACROSTR(a) (_BFS_MACROSTR(a))

/* 用于标识版本号 可以用nm -C查看 */
#ifdef VERSION_TAG
const char *kBFSVersionTag = BFS_MACROSTR(VERSION_TAG);
#else
const char *kBFSVersionTag = "unknown";
#endif

void FileSystem::PrintVersion() noexcept {
  LOG(DEBUG) << "bfs version: " << kBFSVersionTag;
}

const char *FileSystem::GetVersion() const noexcept { return kBFSVersionTag; }

FileSystem::FileSystem(/* args */) { PrintVersion(); }

FileSystem::~FileSystem() {}

/**
 * change master attribute
 *
 * \param master
 *
 * \return true or false
 */
bool FileSystem::set_is_mounted(bool mounted) noexcept {
  mount_stat_.set_is_mounted(mounted);
  return true;
}

/**
 * Wether node has mounted successfully
 *
 * \param void
 *
 * \return true or false
 */
const bool FileSystem::is_mounted() noexcept {
  if (unlikely(!mount_stat_.is_mounted())) {
    LOG(DEBUG) << "mount has not finshed yet";
    errno = EBUSY;
  }
  return mount_stat_.is_mounted();
}

/**
 * time seconds to update atime, mtime, ctme
 *
 * \param void
 *
 * \return second
 */
const int64_t FileSystem::time_update() noexcept {
  // return mount_info_.time_update();
  return 3600;
}

int32_t FileSystem::Sync() {
  LOG(WARNING) << "sync not implemented yet";
  return -1;
}

int64_t FileSystem::SendFile(int32_t out_fd, int32_t in_fd, off_t *offset,
                             int64_t count) {
  LOG(WARNING) << "sendfile not implemented yet";
  return -1;
}

// Create symlink
int32_t FileSystem::Symlink(const char *oldpath, const char *newpath) {
  LOG(WARNING) << "symlink not implemented yet";
  return -1;
}

// Hard Link file src to target.
int32_t FileSystem::LinkFile(const std::string &src,
                             const std::string &target) {
  LOG(WARNING) << "link not implemented yet";
  return -1;
}

int32_t FileSystem::NumFileLinks(const std::string &fname, uint64_t *count) {
  LOG(WARNING) << "number of link not implemented yet";
  return -1;
}

int32_t FileSystem::ReadLink(const char *path, char *buf, size_t size) {
  LOG(WARNING) << "readlink not implemented yet";
  return -1;
}

BLOCKFS_FILE *FileSystem::FileOpen(const char *filename, const char *mode) {
  // LOG(INFO) << "fopen file name: " << filename;
  // int32_t fd = file_handle()->open(filename, O_RDWR, 0);
  // if (fd < 0) {
  //   return nullptr;
  // }
  // BLOCKFS_FILE *f = new BLOCKFS_FILE();
  // f->fd_ = fd;
  // return f;
  LOG(WARNING) << "fopen not implemented yet";
  return nullptr;
}

int32_t FileSystem::FileClose(BLOCKFS_FILE *stream) {
  // LOG(INFO) << "fclose file fd: " << stream->fd_;
  // int32_t ret = block_fs_close(stream->fd_);
  // delete stream;
  // return ret;
  LOG(WARNING) << "fclose not implemented yet";
  return -1;
}

int32_t FileSystem::FileFlush(BLOCKFS_FILE *stream) {
  LOG(WARNING) << "fflush not implemented yet";
  return -1;
}

int32_t FileSystem::FilePutc(int c, BLOCKFS_FILE *stream) {
  LOG(WARNING) << "fputc not implemented yet";
  return -1;
}

int32_t FileSystem::FileGetc(BLOCKFS_FILE *stream) {
  LOG(WARNING) << "fgetc not implemented yet";
  return -1;
}

int32_t FileSystem::FilePuts(const char *str, BLOCKFS_FILE *stream) {
  LOG(WARNING) << "fputs not implemented yet";
  return -1;
}

char *FileSystem::FileGets(char *str, int32_t n, BLOCKFS_FILE *stream) {
  LOG(WARNING) << "fgets not implemented yet";
  return nullptr;
}

// https://github.com/LLNL/cruise/blob/master/src/cruise-sysio.c
int32_t FileSystem::FileSeek(BLOCKFS_FILE *stream, int64_t offset, int whence) {
  // stream->offset_ = block_fs_lseek(stream->fd_, offset, whence);
  // return stream->offset_;
  LOG(WARNING) << "fseek not implemented yet";
  return -1;
}

int32_t FileSystem::FilePrintf(BLOCKFS_FILE *stream, const char *format, ...) {
  LOG(WARNING) << "fprintf not implemented yet";
  return -1;
}

int32_t FileSystem::FileScanf(BLOCKFS_FILE *stream, const char *format, ...) {
  LOG(WARNING) << "fscanf not implemented yet";
  return -1;
}

int32_t FileSystem::FileEof(BLOCKFS_FILE *stream) {
  LOG(WARNING) << "feof not implemented yet";
  return -1;
}

int32_t FileSystem::FileError(BLOCKFS_FILE *stream) {
  LOG(WARNING) << "ferror not implemented yet";
  return -1;
}

int32_t FileSystem::FileRewind(BLOCKFS_FILE *stream) {
  LOG(WARNING) << "frewind not implemented yet";
  return -1;
}

int32_t FileSystem::FileRead(void *ptr, size_t size, size_t nmemb,
                             BLOCKFS_FILE *stream) {
  // int64_t ret = block_fs_pread(stream->fd_, const_cast<void *>(ptr),
  //                              size * nmemb, stream->offset_);
  // stream->offset_ += ret;
  // return ret;
  LOG(WARNING) << "fread not implemented yet";
  return -1;
}

int32_t FileSystem::FileWrite(const void *ptr, size_t size, size_t nmemb,
                              BLOCKFS_FILE *stream) {
  // int64_t ret = block_fs_pwrite(stream->fd_, const_cast<void *>(ptr),
  //                               size * nmemb, stream->offset_);
  // stream->offset_ += ret;
  // return ret;
  LOG(WARNING) << "fwrite not implemented yet";
  return -1;
}

// https://blog.csdn.net/qq_41453285/article/details/90750078?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase
// https://blog.csdn.net/qq_31833457/article/details/81571729
// http://blog.chinaunix.net/uid-30025978-id-4691526.html
// https://baike.baidu.com/item/mkstemp%28%29/3672735?fr=aladdin

// 创建名字唯一的临时文件：推荐使用tmpfile和mkstemp安全函数
// Mkstemp()会以可读写模式 和0600
// 权限来打开该文件,如果该文件不存在则会建立该文件
// 参数template所指的文件名称字符串必须声明为数组
// char template[ ] =”template-XXXXXX”;
int32_t FileSystem::MksTemp(char *template_str) {
  // return file_handle()->mkstemp(template_str);
  LOG(WARNING) << "mkstemp not implemented yet";
  return -1;
}

BLOCKFS_FILE *FileSystem::TmpFile(void) {
  LOG(WARNING) << "tmpfile not implemented yet";
  return nullptr;
}

// tmpnam 为什么不安全?
// 返回一个临时文件的路径，但并不创建该临时文件
char *FileSystem::MkTemp(char *template_str) {
  LOG(WARNING) << "mktemp not implemented yet";
  return nullptr;
}

// https://segmentfault.com/q/1010000003833979/a-1020000003834795
char *FileSystem::TmpNam(char *str) {
  LOG(WARNING) << "tmpnam not implemented yet";
  return nullptr;
}

}  // namespace blockfs
}  // namespace udisk