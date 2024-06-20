#include <assert.h>

#include "crc.h"
#include "file_system.h"
#include "logging.h"
#include "spdlog/spdlog.h"

namespace udisk::blockfs {

const static FilePtr kEmptyFilePtr;
const static OpenFilePtr kEmptyOpenFilePtr;

bool FileHandle::InitializeMeta() {
  FileMeta *meta;
  for (ino_t fh = 0;
       fh < FileSystem::Instance()->super_meta()->max_file_num; ++fh) {
    meta = reinterpret_cast<FileMeta *>(base_addr() +
        FileSystem::Instance()->super_meta()->file_meta_size_ * fh);
    uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                         FileSystem::Instance()->super_meta()->file_meta_size_ -
                             sizeof(meta->crc_));
    if (meta->crc_ != crc) [[unlikely]] {
      LOG(ERROR) << "file meta " << fh << " crc error, read:" << meta->crc_
                 << " cal: " << crc << " file name: " << meta->file_name_;
      return false;
    }
    if (meta->used_) {
      // 临时文件直接清理掉
      if (meta->is_temp_) {
        LOG(WARNING) << "clean temp file handle: " << meta->fh_;
        File::ClearMeta(meta);
        if (!File::WriteMeta(meta->fh_)) {
          return false;
        }
        continue;
      }
      LOG(DEBUG) << "file handle: " << fh << " name: " << meta->file_name_
                 << " size: " << meta->size_ << " dh: " << meta->dh_
                 << " child_fh: " << meta->child_fh_
                 << " parent_fh: " << meta->parent_fh_;
      if (::strnlen(meta->file_name_, sizeof(meta->file_name_)) ==
                   0) [[unlikely]] {
        SPDLOG_ERROR("file meta {} used but name empty", fh);
        return false;
      }
      if (meta->fh_ != fh) [[unlikely]] {
        SPDLOG_ERROR("file meta {} used but fh invalid", fh);
        return false;
      }

      FilePtr file = std::make_shared<File>(meta);
      if (!file) return false;
      if (file->child_fh() > 0) {
        SPDLOG_DEBUG("this is parent file, only add to fh map");
        created_fhs_[file->fh()] = file;
        // ParentFilePtr parent = ParentFile::NewParentFile();
      } else {
        // LOG(WARNING) << "This is child file, add to fh and name map";
        AddFileNoLock(file);
      }

      // DH索引只在加载元数据时查找文件夹, 因为文件没有记录文件夹的绝对路径
      const DirectoryPtr &dir =
          FileSystem::Instance()->dir_handle()->GetCreatedDirectoryNolock(
              meta->dh_);
      if (!dir) [[unlikely]] {
        LOG(ERROR) << "file meta " << fh << " cannot find parent directory";
        return false;
      }
      dir->AddChildFileNoLock(file);
    } else {
      AddFile2FreeNolock(fh);
    }
  }
  // TODO: 全部加入到文件列表, 最后来过滤一遍, 去掉child_fh
  SPDLOG_DEBUG("read file meta success, free num: {}", free_meta_size());
  return true;
}

bool FileHandle::FormatAllMeta() {
  uint64_t file_meta_total_size =
      FileSystem::Instance()->super_meta()->file_meta_total_size;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      file_meta_total_size, FileSystem::Instance()->dev()->block_size());

  FileMeta *meta;
  for (ino_t fh = 0;
       fh < FileSystem::Instance()->super_meta()->max_file_num; ++fh) {
    meta = reinterpret_cast<FileMeta *>(
        buffer->data() +
        FileSystem::Instance()->super_meta()->file_meta_size_ * fh);
    meta->used_ = false;
    meta->fh_ = fh;
    meta->child_fh_ = -1;
    meta->parent_fh_ = -1;
    meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                       FileSystem::Instance()->super_meta()->file_meta_size_ -
                           sizeof(meta->crc_));
  }
  int64_t ret = FileSystem::Instance()->dev()->PwriteDirect(
      buffer->data(),
      FileSystem::Instance()->super_meta()->file_meta_total_size,
      FileSystem::Instance()->super_meta()->file_meta_offset_);
  if (ret != static_cast<int64_t>(
                 FileSystem::Instance()->super_meta()->file_meta_total_size)) {
    SPDLOG_ERROR("write file meta error size: {}", ret);
    return false;
  }
  SPDLOG_INFO("write all file meta success");
  return true;
}

/**
 * 文件的全路径转换成文件夹路径和文件名字
 *
 * \param filename UXDB-UDISK约定: DB创建的绝对目录
 * \param new_dirname 文件夹的绝对目录
 * \param new_filename 文件的名字
 */
bool FileHandle::TransformPath(const std::string &filename,
                               std::string &new_dirname,
                               std::string &new_filename) {
  if (filename[filename.size() - 1] == '/') [[unlikely]] {
    SPDLOG_ERROR("file cannot endwith dir separator: {}", filename);
    errno = ENOTDIR;
    return false;
  }
  new_dirname = GetDirName(filename);
  new_filename = GetFileName(filename);
  SPDLOG_INFO("transformPath dirname: {}, filename: {}", new_dirname, new_filename);
  if (new_filename.size() >= kBlockFsMaxFileNameLen) [[unlikely]] {
    SPDLOG_ERROR("file name exceed size: {}", new_filename);
    errno = ENAMETOOLONG;
    return false;
  }
  return true;
}

/**
 * Truncate申请 一个新的meta, 但是文件智能指针没变
 *
 * \param dh 父文件夹的句柄
 * \param filename 文件的名字, 不带前面的路径
 *
 * \return 文件meta结构体的指针
 */
FileMeta *FileHandle::NewFreeFileMeta(int32_t dh, const std::string &filename) {
  META_HANDLE_LOCK();
  if (free_fhs_.empty()) [[unlikely]] {
    SPDLOG_WARN("file meta not enough");
    errno = ENFILE;
    return nullptr;
  }
  ino_t fh = free_fhs_.front();
  SPDLOG_INFO("create new file handle: {}", fh);

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (meta->used_ || meta->fh_ != fh || meta->size_ != 0) [[unlikely]] {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    errno = EINVAL;
    return nullptr;
  }
  meta->used_ = true;
  meta->fh_ = fh;
  meta->dh_ = dh;
  ::memset(meta->file_name_, 0, sizeof(meta->file_name_));
  ::memcpy(meta->file_name_, filename.c_str(), filename.size());

  free_fhs_.pop_front();

  return meta;
}

FilePtr FileHandle::NewFreeFileNolock(int32_t dh, const std::string &filename) {
  if (free_fhs_.empty()) [[unlikely]] {
    SPDLOG_ERROR("file meta not enough");
    errno = ENFILE;
    return kEmptyFilePtr;
  }
  ino_t fh = free_fhs_.front();
  SPDLOG_INFO("create new file handle: {} name: {}", fh, filename);

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (meta->used_ || meta->fh_ != fh || meta->size_ != 0) [[unlikely]] {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    errno = EINVAL;
    return kEmptyFilePtr;
  }

  FilePtr file = std::make_shared<File>();
  if (!file) {
    SPDLOG_ERROR("failed to new file pointer");
    errno = ENOMEM;
    return kEmptyFilePtr;
  }
  file->set_meta(meta);
  file->set_used(true);
  file->set_fh(fh);
  file->set_dh(dh);
  file->set_file_name(filename);
  file->set_is_temp(false);

  free_fhs_.pop_front();

  return file;
}

FilePtr FileHandle::NewFreeTmpFileNolock(int32_t dh) {
  if (free_fhs_.empty()) [[unlikely]] {
    SPDLOG_WARN("file meta not enough");
    errno = ENFILE;
    return nullptr;
  }
  ino_t fh = free_fhs_.front();
  std::string tmp_file_name = "tmpfile_" + std::to_string(fh);
  SPDLOG_INFO("create new tmp file handle: {} name: {}", fh, tmp_file_name);

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (meta->used_ || meta->fh_ != fh || meta->size_ != 0) [[unlikely]] {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    errno = EINVAL;
    return nullptr;
  }

  FilePtr file = std::make_shared<File>();
  if (!file) {
    SPDLOG_ERROR("failed to new file pointer");
    errno = ENOMEM;
    return nullptr;
  }
  file->set_meta(meta);
  file->set_used(true);
  file->set_fh(fh);
  file->set_dh(dh);
  file->set_file_name(tmp_file_name);
  file->set_is_temp(true);

  free_fhs_.pop_front();

  return file;
}

bool FileHandle::NewFile(const std::string &dirname,
                         const std::string &filename, bool tmpfile,
                         std::pair<DirectoryPtr, FilePtr> *dirs) {
  const DirectoryPtr &dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dirname);
  if (!dir) [[unlikely]] {
    SPDLOG_ERROR("parent directory not exist: {}", dirname);
    errno = ENOENT;
    return false;
  }
  // 找到父文件夹
  dirs->first = dir;

  META_HANDLE_LOCK();
  if (!tmpfile) {
    FileNameKey key = std::make_pair(dir->dh(), filename);
    auto itor = created_files_.find(key);
    if (itor != created_files_.end()) [[unlikely]] {
      SPDLOG_WARN("file has already exist: {} {}", dirname, filename);
      errno = EEXIST;
      return false;
    }
    FilePtr file = NewFreeFileNolock(dir->dh(), filename);
    if (!file) {
      return false;
    }
    dirs->second = file;
  } else {
    FilePtr file = NewFreeTmpFileNolock(dir->dh());
    if (!file) {
      return false;
    }
    dirs->second = file;
  }
  time_t time = ::time(nullptr);
  dirs->second->set_atime(time);
  dirs->second->set_mtime(time);
  dirs->second->set_ctime(time);
  return true;
}

void FileHandle::AddFileNoLock(const FilePtr &file) noexcept {
  created_fhs_[file->fh()] = file;
  FileNameKey key = std::make_pair(file->dh(), file->file_name());
  created_files_[key] = file;
}

void FileHandle::AddFileToDirectory(const DirectoryPtr &parent,
                                    const FilePtr &file) {
  parent->AddChildFile(file);

  META_HANDLE_LOCK();
  AddFileNoLock(file);
}

bool FileHandle::RemoveFileNoLock(const FilePtr &file) noexcept {
  FileNameKey key = std::make_pair(file->dh(), file->file_name());
  if (created_files_.erase(key) != 1) {
    SPDLOG_ERROR("failed to remove file name map: {}", file->file_name());
    return false;
  }
  if (created_fhs_.erase(file->fh()) != 1) {
    SPDLOG_ERROR("failed to remove file fh map: {}", file->file_name());
    return false;
  }
  return true;
}

bool FileHandle::RemoveFileFromoDirectory(const DirectoryPtr &parent,
                                          const FilePtr &file) {
  if (!parent->RemoveChildFile(file)) {
    return false;
  }
  META_HANDLE_LOCK();
  return RemoveFileNoLock(file);
}

bool FileHandle::RemoveFileFromoDirectoryNolock(const DirectoryPtr &parent,
                                                const FilePtr &file) {
  if (!parent->RemoveChildFile(file)) {
    return false;
  }
  return RemoveFileNoLock(file);
}

bool FileHandle::AddParentFile(const ParentFilePtr &parent) {
  META_HANDLE_LOCK();
  auto it = parent_files_.find(parent->fh());
  if (it != parent_files_.end()) [[unlikely]] {
    return false;
  }
  parent_files_[parent->fh()] = parent;
  return true;
}

bool FileHandle::RemoveParentFile(ino_t fh) {
  META_HANDLE_LOCK();
  auto it = parent_files_.find(fh);
  if (it == parent_files_.end()) [[unlikely]] {
    return false;
  }
  ParentFilePtr parent = (ParentFilePtr)it->second;
  parent_files_.erase(fh);
  return parent->Recycle();
}

void FileHandle::AddFile2FreeNolock(int32_t index) noexcept {
  free_fhs_.push_back(index);
}

void FileHandle::AddFile2Free(int32_t index) {
  META_HANDLE_LOCK();
  AddFile2FreeNolock(index);
}

// 通过索引找到对齐的4K页面的
uint32_t FileHandle::PageAlignIndex(uint32_t index) {
  static const uint32_t FILE_ALIGN =
      kBlockFsPageSize / FileSystem::Instance()->super_meta()->file_meta_size_;
  return index - (index % FILE_ALIGN);
}

bool FileHandle::FindFile(const std::string &dirname,
                          const std::string &filename,
                          std::pair<DirectoryPtr, FilePtr> *dirs) {
  const DirectoryPtr &dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(dirname);
  if (!dir) [[unlikely]] {
    LOG(ERROR) << "parent directory not exist: " << dirname;
    errno = ENOENT;
    return false;
  }
  // 找到父文件夹
  dirs->first = dir;

  META_HANDLE_LOCK();
  FileNameKey key = std::make_pair(dir->dh(), filename);
  auto itor = created_files_.find(key);
  if (itor == created_files_.end()) [[unlikely]] {
    LOG(WARNING) << "file not exist: " << dirname << filename;
    errno = ENOENT;
    return false;
  }
  dirs->second = itor->second;
  return true;
}

bool FileHandle::UpdateMeta() {
  META_HANDLE_LOCK();
  for (const auto &it : created_files_) {
    if (!it.second->UpdateMeta()) {
      return false;
    }
  }
  return true;
}

FilePtr FileHandle::CreateFile(const std::string &filename, mode_t mode,
                               bool tmpfile) {
  std::string new_dirname;
  std::string new_filename;
  if (!tmpfile) {
    SPDLOG_INFO("create file: {}", filename);
    if (!TransformPath(filename, new_dirname, new_filename)) {
      return kEmptyFilePtr;
    }
  } else {
    // 创建临时文件传入的参数是文件目录
    new_dirname = filename;
    SPDLOG_INFO("create tmp file in directory: {}", new_dirname);
  }
  std::pair<DirectoryPtr, FilePtr> dirs;
  if (!NewFile(new_dirname, new_filename, tmpfile, &dirs)) [[unlikely]] {
    return kEmptyFilePtr;
  }
  DirectoryPtr parent_dir = dirs.first;
  FilePtr curr_file = dirs.second;

  {
    META_HANDLE_LOCK();
    // 持久化文件元数据
    if (!curr_file->WriteMeta()) {
      // TODO:失败处理
      return kEmptyFilePtr;
    }
  }

  // 添加到内存文件列表和父文件夹中
  AddFileToDirectory(parent_dir, curr_file);

  SPDLOG_INFO("create file success: {}", curr_file->file_name());
  errno = 0;
  return curr_file;
}

int FileHandle::UnlinkFileNolock(const ino_t fh) {
  SPDLOG_INFO("unlink file handle: {}", fh);
  FilePtr file = GetCreatedFileNoLock(fh);
  if (!file) {
    errno = ENOENT;
    return -1;
  }

  const DirectoryPtr &parent_dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectoryNolock(
          file->dh());
  if (!parent_dir) {
    errno = EISDIR;
    return -1;
  }

  time_t time = ::time(nullptr);
  file->set_mtime(time);
  file->set_ctime(time);

  if (file->LinkCount() > 0) {
    return -1;
  }

  // 持久化删除fileblock
  if (!file->RemoveAllFileBlock()) {
    return -1;
  }

  // 持久化删除文件
  if (!file->ReleaseNolock()) {
    // TODO:失败处理
    return -1;
  }

  if (!RemoveFileFromoDirectoryNolock(parent_dir, file)) {
    return -1;
  }

  SPDLOG_INFO("remove file success: {}", file->file_name());
  errno = 0;
  return 0;
}

int FileHandle::unlink(const std::string &filename) {
  SPDLOG_INFO("unlink file: {}", filename);

  const DirectoryPtr &dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(filename);
  if (dir) {
    errno = EISDIR;
    return -1;
  }

  std::string new_dirname;
  std::string new_filename;
  if (!TransformPath(filename, new_dirname, new_filename)) {
    return -1;
  }

  std::pair<DirectoryPtr, FilePtr> dirs;
  if (!FindFile(new_dirname, new_filename, &dirs)) [[unlikely]] {
    return -1;
  }
  DirectoryPtr parent_dir = dirs.first;
  FilePtr curr_file = dirs.second;

  if (curr_file->LinkCount() > 0) {
    SPDLOG_WARN("file count not zero: {}", curr_file->LinkCount());
    curr_file->set_deleted(true);
    return 0;
  }

  // 持久化删除fileblock
  if (!curr_file->RemoveAllFileBlock()) {
    return -1;
  }

  // 持久化删除文件
  if (!curr_file->Release()) {
    // TODO:失败处理
    return -1;
  }

  if (!RemoveFileFromoDirectory(parent_dir, curr_file)) {
    return -1;
  }

  SPDLOG_INFO("remove file success: {}", filename);
  errno = 0;
  return 0;
}

bool FileHandle::CheckFileExist(const std::string &path) {
  return (GetCreatedFile(path) == kEmptyFilePtr);
}

const OpenFilePtr &FileHandle::GetOpenFile(ino_t fd) {
  META_HANDLE_LOCK();
  return GetOpenFileNolock(fd);
}

const OpenFilePtr &FileHandle::GetOpenFileNolock(ino_t fd) {
  auto itor = open_files_.find(fd);
  if (itor == open_files_.end()) [[unlikely]] {
    SPDLOG_WARN("fd not been opened: {}", fd);
    errno = EBADF;
    return kEmptyOpenFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFile(int32_t fh) {
  META_HANDLE_LOCK();
  return GetCreatedFileNoLock(fh);
}

const FilePtr &FileHandle::GetCreatedFileNoLock(ino_t fh) {
  auto itor = created_fhs_.find(fh);
  if (itor == created_fhs_.end()) {
    SPDLOG_ERROR("file handle not exist: {}", fh);
    return kEmptyFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFile(const std::string &filename) {
  SPDLOG_INFO("get created file name: {}", filename);
  std::string new_dirname;
  std::string new_filename;
  if (!TransformPath(filename, new_dirname, new_filename)) {
    return kEmptyFilePtr;
  }
  const DirectoryPtr &dir =
      FileSystem::Instance()->dir_handle()->GetCreatedDirectory(new_dirname);
  if (!dir) [[unlikely]] {
    return kEmptyFilePtr;
  }
  std::lock_guard lock(mutex_);
  FileNameKey key = std::make_pair(dir->dh(), new_filename);
  return GetCreatedFileNoLock(key);
}

const FilePtr &FileHandle::GetCreatedFileNoLock(const FileNameKey &key) {
  auto itor = created_files_.find(key);
  if (itor == created_files_.end()) [[unlikely]] {
    SPDLOG_WARN("file not exist: {}", key.second);
    /* ERROR: trying to open a file that does not exist without O_CREATE */
    errno = ENOENT;
    return kEmptyFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFileNolock(int32_t dh,
                                                const std::string &filename) {
  auto itor = created_files_.find(std::make_pair(dh, filename));
  if (itor == created_files_.end()) [[unlikely]] {
    SPDLOG_WARN("file not exist: {}", filename);
    return kEmptyFilePtr;
  }
  return itor->second;
}

static bool VerifyOpenExistFileFlag(const int32_t flags) {
  /* trying to open a file that exists with O_CREATE and O_EXCL
   * if O_CREAT and O_EXCL are set, this is an error */
  if ((flags & O_CREAT) && (flags & O_EXCL)) {
    errno = EEXIST;
    return false;
  }
  /* if O_DIRECTORY is set and fid is not a directory, error */
  if ((flags & O_DIRECTORY)) {
    errno = ENOTDIR;
    return false;
  }
  return true;
}

static bool IsNeedCreateTempFile(const int32_t flags) {
  if (!(flags & O_TMPFILE)) {
    return false;
  }
  if (!(flags & O_RDWR) && !(flags & O_WRONLY)) {
    SPDLOG_ERROR("invalid tmp open flag without O_RDWR or O_WRONLY");
    errno = EINVAL;
    return false;
  }
  return true;
}

int FileHandle::open(const std::string &filename, int32_t flags, mode_t mode) {
  FilePtr file;
  if (IsNeedCreateTempFile(flags)) {
    // when create temp file, the filename is dirname
    file = CreateFile(filename, flags, true);
    if (!file) {
      return -1;
    }
  } else {
    if (FileSystem::Instance()->dir_handle()->GetCreatedDirectory(filename)) {
      errno = EISDIR;
      return -1;
    }
    file = GetCreatedFile(filename);
    if (!file) {
      if (flags & O_CREAT) {
        file = CreateFile(filename, flags);
        if (!file) {
          return -1;
        }
      } else {
        errno = ENOENT;
        return -1;
      }
    } else {
      /* file already exists */
      if (!VerifyOpenExistFileFlag(flags)) {
        return -1;
      }
      if ((flags & O_TRUNC) && (flags & (O_RDWR | O_WRONLY))) {
        if (file->ftruncate(0) < 0) {
          return -1;
        }
      }
    }
  }

  /* allocate a file fd for this new file */
  ino_t fd = FileSystem::Instance()->fd_handle()->get_fd();
  if (fd < 0) {
    return -1;
  }
  OpenFilePtr open_file = std::make_shared<OpenFile>(file);
  AddOpenFile(fd, open_file);
  SPDLOG_INFO("open file name: {}, fd: {}, fh: {}", filename, fd, file->fh());

  if (flags & O_APPEND) {
    // if O_APPEND is set, we need to place file pointer at end of file
    open_file->set_append_pos(file->file_size());
  } else {
    open_file->set_append_pos(0);
  }
  file->IncLinkCount();

  errno = 0;
  return fd;
}

int FileHandle::close(ino_t fd) noexcept {
  std::lock_guard lock(mutex_);
  const OpenFilePtr &open_file = GetOpenFileNolock(fd);
  if (!open_file) [[unlikely]] {
    return -1;
  }
  const FilePtr &file = open_file->file();
  file->fsync();
  SPDLOG_INFO("close file name: {}, fd: {}", file->file_name(), fd);
  // 文件关闭的时候去掉文件锁
  file->set_locked(false);
  file->DecLinkCount();
  // TODO:临时文件或者文件已经被删除, 引用为0自动删除
  if (file->LinkCount() == 0) {
    if (file->is_temp()) {
      SPDLOG_WARN("clean temp file fh: {}", file->fh());
      UnlinkFileNolock(file->fh());
    } else if (file->deleted()) {
      SPDLOG_WARN("clean deleted file fh: {}", file->fh());
      UnlinkFileNolock(file->fh());
    }
  }
  // keep the reference of file pointer
  open_files_.erase(fd);
  FileSystem::Instance()->fd_handle()->put_fd(fd);
  errno = 0;
  return 0;
}

int FileHandle::dup(int oldfd) {
  const OpenFilePtr &old_open_file = GetOpenFile(oldfd);
  if (!old_open_file) {
    // errno = ENOENT;
    return -1;
  }
  const FilePtr &file = old_open_file->file();

  /* allocate a file fd for this new file */
  ino_t newfd = FileSystem::Instance()->fd_handle()->get_fd();
  if (newfd < 0) {
    return -1;
  }
  OpenFilePtr new_open_file = std::make_shared<OpenFile>(file);
  AddOpenFile(newfd, new_open_file);
  SPDLOG_INFO("dup oldfd: {}, newfd: {}, fh: {}", oldfd, newfd, file->fh());
  file->IncLinkCount();

  errno = 0;
  return newfd;
}

int FileHandle::fsync(ino_t fd) {
  const OpenFilePtr &open_file = GetOpenFile(fd);
  if (!open_file) {
    errno = ENOENT;
    return -1;
  }
  return open_file->file()->fsync();
}

void FileHandle::Dump() noexcept {
  for (auto &kv : created_files_) {
    const FileNameKey &key = kv.first;
    LOG(INFO) << "find dh: " << key.first << " name: " << key.second
              << " name: " << kv.second->file_name();
  }
  for (auto &[fh, ptr] : created_fhs_) {
    SPDLOG_INFO("find fh: {} name: {}", fh, ptr->file_name());
  }
}

void FileHandle::Dump(const std::string &file_name) noexcept {}

}