#include <assert.h>

#include "crc.h"
#include "file_store_udisk.h"
#include "logging.h"

namespace udisk::blockfs {

const static FilePtr kEmptyFilePtr;
const static OpenFilePtr kEmptyOpenFilePtr;

bool FileHandle::RunInMetaGuard(const FileModifyCallback &cb) {
  META_HANDLE_LOCK();
  return cb();
}

bool FileHandle::InitializeMeta() {
  FileMeta *meta;
  for (uint64_t fh = 0;
       fh < FileStore::Instance()->super_meta()->max_file_num; ++fh) {
    meta = reinterpret_cast<FileMeta *>(base_addr() +
        FileStore::Instance()->super_meta()->file_meta_size_ * fh);
    uint32_t crc = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                         FileStore::Instance()->super_meta()->file_meta_size_ -
                             sizeof(meta->crc_));
    if (unlikely(meta->crc_ != crc)) {
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
                 << " seq_no: " << meta->seq_no_
                 << " child_fh: " << meta->child_fh_
                 << " parent_fh: " << meta->parent_fh_;
      if (unlikely(::strnlen(meta->file_name_, sizeof(meta->file_name_)) ==
                   0)) {
        LOG(ERROR) << "file meta " << fh << " used but name empty";
        return false;
      }
      if (unlikely(meta->fh_ != static_cast<int32_t>(fh))) {
        LOG(ERROR) << "file meta " << fh << " used but fh invalid";
        return false;
      }

      FilePtr file = std::make_shared<File>(meta);
      if (!file) return false;
      if (file->child_fh() > 0) {
        LOG(DEBUG) << "this is parent file, only add to fh map";
        AddFileHandle(file);
        // ParentFilePtr parent = ParentFile::NewParentFile();
      } else {
        // LOG(WARNING) << "This is child file, add to fh and name map";
        AddFileNoLock(file);
      }

      // DH索引只在加载元数据时查找文件夹, 因为文件没有记录文件夹的绝对路径
      const DirectoryPtr &dir =
          FileStore::Instance()->dir_handle()->GetCreatedDirectoryNolock(
              meta->dh_);
      if (unlikely(!dir)) {
        LOG(ERROR) << "file meta " << fh << " cannot find parent directory";
        return false;
      }
      dir->AddChildFileNoLock(file);
    } else {
      AddFile2FreeNolock(fh);
    }
  }
  // TODO: 全部加入到文件列表, 最后来过滤一遍, 去掉child_fh
  LOG(DEBUG) << "read file meta success, free num:" << free_meta_size();
  return true;
}

bool FileHandle::FormatAllMeta() {
  uint64_t file_meta_total_size =
      FileStore::Instance()->super_meta()->file_meta_total_size_;
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      file_meta_total_size, FileStore::Instance()->dev()->block_size());

  FileMeta *meta;
  for (uint64_t fh = 0;
       fh < FileStore::Instance()->super_meta()->max_file_num; ++fh) {
    meta = reinterpret_cast<FileMeta *>(
        buffer->data() +
        FileStore::Instance()->super_meta()->file_meta_size_ * fh);
    meta->used_ = false;
    meta->fh_ = fh;
    meta->child_fh_ = -1;
    meta->parent_fh_ = -1;
    meta->crc_ = Crc32(reinterpret_cast<uint8_t *>(meta) + sizeof(meta->crc_),
                       FileStore::Instance()->super_meta()->file_meta_size_ -
                           sizeof(meta->crc_));
  }
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      buffer->data(),
      FileStore::Instance()->super_meta()->file_meta_total_size_,
      FileStore::Instance()->super_meta()->file_meta_offset_);
  if (ret != static_cast<int64_t>(
                 FileStore::Instance()->super_meta()->file_meta_total_size_)) {
    LOG(ERROR) << "write file meta error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all file meta success";
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
  std::string file_name = filename;
  if (!FileStore::Instance()->super()->CheckMountPoint(file_name, true)) {
    return false;
  }
  new_dirname = GetDirName(file_name);
  new_filename = GetFileName(file_name);
  LOG(INFO) << "transformPath dirname: " << new_dirname;
  LOG(INFO) << "transformPath filename: " << new_filename;
  if (new_filename.size() >= kBlockFsMaxFileNameLen) [[unlikely]] {
    LOG(ERROR) << "file name exceed size: " << new_filename;
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
  if (unlikely(free_metas_.empty())) {
    LOG(WARNING) << "file meta not enough";
    block_fs_set_errno(ENFILE);
    return nullptr;
  }
  int32_t fh = free_metas_.front();
  LOG(INFO) << "create new file handle: " << fh;

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (unlikely(meta->used_ || meta->fh_ != fh || meta->size_ != 0)) {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    block_fs_set_errno(EINVAL);
    return nullptr;
  }
  meta->used_ = true;
  meta->fh_ = fh;
  meta->dh_ = dh;
  ::memset(meta->file_name_, 0, sizeof(meta->file_name_));
  ::memcpy(meta->file_name_, filename.c_str(), filename.size());

  free_metas_.pop_front();

  return meta;
}

FilePtr FileHandle::NewFreeFileNolock(int32_t dh, const std::string &filename) {
  if (free_metas_.empty()) [[unlikely]] {
    LOG(ERROR) << "file meta not enough";
    errno = ENFILE;
    return kEmptyFilePtr;
  }
  int32_t fh = free_metas_.front();
  LOG(INFO) << "create new file handle: " << fh << " name: " << filename;

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (unlikely(meta->used_ || meta->fh_ != fh || meta->size_ != 0)) {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    errno = EINVAL;
    return kEmptyFilePtr;
  }

  FilePtr file = std::make_shared<File>();
  if (!file) {
    LOG(ERROR) << "failed to new file pointer";
    errno = ENOMEM;
    return kEmptyFilePtr;
  }
  file->set_meta(meta);
  file->set_used(true);
  file->set_fh(fh);
  file->set_dh(dh);
  file->set_file_name(filename);
  file->set_is_temp(false);

  free_metas_.pop_front();

  return file;
}

FilePtr FileHandle::NewFreeTmpFileNolock(int32_t dh) {
  if (unlikely(free_metas_.empty())) {
    LOG(WARNING) << "file meta not enough";
    block_fs_set_errno(ENFILE);
    return nullptr;
  }
  int32_t fh = free_metas_.front();
  std::string tmp_file_name = "tmpfile_" + std::to_string(fh);
  LOG(INFO) << "create new tmp file handle: " << fh
            << " name: " << tmp_file_name;

  FileMeta *meta = reinterpret_cast<FileMeta *>(
      base_addr() + sizeof(FileMeta) * fh);
  if (unlikely(meta->used_ || meta->fh_ != fh || meta->size_ != 0)) {
    LOG(ERROR) << "new file meta invalid, used: " << meta->used_
               << " size: " << meta->size_ << " fh: " << meta->fh_
               << " wanted fh: " << fh;
    block_fs_set_errno(EINVAL);
    return nullptr;
  }

  FilePtr file = std::make_shared<File>();
  if (!file) {
    LOG(ERROR) << "failed to new file pointer";
    block_fs_set_errno(ENOMEM);
    return nullptr;
  }
  file->set_meta(meta);
  file->set_used(true);
  file->set_fh(fh);
  file->set_dh(dh);
  file->set_file_name(tmp_file_name);
  file->set_is_temp(true);

  free_metas_.pop_front();

  return file;
}

bool FileHandle::NewFile(const std::string &dirname,
                         const std::string &filename, bool tmpfile,
                         std::pair<DirectoryPtr, FilePtr> *dirs) {
  const DirectoryPtr &dir =
      FileStore::Instance()->dir_handle()->GetCreatedDirectory(dirname);
  if (!dir) [[unlikely]] {
    LOG(ERROR) << "parent directory not exist: " << dirname;
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
      LOG(WARNING) << "file has already exist: " << dirname << filename;
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

void FileHandle::AddFileHandle(const FilePtr &file) noexcept {
  created_fhs_[file->fh()] = file;
}

// 文件名不能保证唯一, 文件dh+文件名可以
void FileHandle::AddFileName(const FilePtr &file) noexcept {
  FileNameKey key = std::make_pair(file->dh(), file->file_name());
  created_files_[key] = file;
}

void FileHandle::AddFileNoLock(const FilePtr &file) noexcept {
  AddFileHandle(file);
  AddFileName(file);
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
    LOG(ERROR) << "failed to remove file name map: " << file->file_name();
    return false;
  }
  if (created_fhs_.erase(file->fh()) != 1) {
    LOG(ERROR) << "failed to remove file fh map: " << file->file_name();
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
  if (unlikely(it != parent_files_.end())) {
    return false;
  }
  parent_files_[parent->fh()] = parent;
  return true;
}

bool FileHandle::RemoveParentFile(int32_t fh) {
  META_HANDLE_LOCK();
  auto it = parent_files_.find(fh);
  if (unlikely(it == parent_files_.end())) {
    return false;
  }
  ParentFilePtr parent = (ParentFilePtr)it->second;
  parent_files_.erase(fh);
  return parent->Recycle();
}

bool FileHandle::RemoveParentFile(const ParentFilePtr &parent) {
  return RemoveParentFile(parent->fh());
}

void FileHandle::AddFile2FreeNolock(int32_t index) noexcept {
  free_metas_.push_back(index);
}

void FileHandle::AddFile2Free(int32_t index) {
  META_HANDLE_LOCK();
  AddFile2FreeNolock(index);
}

// 通过索引找到对齐的4K页面的
uint32_t FileHandle::PageAlignIndex(uint32_t index) {
  static const uint32_t FILE_ALIGN =
      kBlockFsPageSize / FileStore::Instance()->super_meta()->file_meta_size_;
  return index - (index % FILE_ALIGN);
}

bool FileHandle::FindFile(const std::string &dirname,
                          const std::string &filename,
                          std::pair<DirectoryPtr, FilePtr> *dirs) {
  const DirectoryPtr &dir =
      FileStore::Instance()->dir_handle()->GetCreatedDirectory(dirname);
  if (unlikely(!dir)) {
    LOG(ERROR) << "parent directory not exist: " << dirname;
    block_fs_set_errno(ENOENT);
    return false;
  }
  // 找到父文件夹
  dirs->first = dir;

  META_HANDLE_LOCK();
  FileNameKey key = std::make_pair(dir->dh(), filename);
  auto itor = created_files_.find(key);
  if (unlikely(itor == created_files_.end())) {
    LOG(WARNING) << "file not exist: " << dirname << filename;
    block_fs_set_errno(ENOENT);
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
    LOG(INFO) << "create file: " << filename;
    if (!TransformPath(filename, new_dirname, new_filename)) {
      return kEmptyFilePtr;
    }
  } else {
    // 创建临时文件传入的参数是文件目录
    new_dirname = filename;
    LOG(INFO) << "create tmp file in directory: " << new_dirname;
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

  LOG(INFO) << "create file success: " << curr_file->file_name();
  errno = 0;
  return curr_file;
}

int FileHandle::UnlinkFile(const int32_t fh) {
  META_HANDLE_LOCK();
  return UnlinkFileNolock(fh);
}

int FileHandle::UnlinkFileNolock(const int fh) {
  LOG(INFO) << "unlink file handle: " << fh;
  FilePtr file = GetCreatedFileNoLock(fh);
  if (!file) {
    block_fs_set_errno(ENOENT);
    return -1;
  }

  const DirectoryPtr &parent_dir =
      FileStore::Instance()->dir_handle()->GetCreatedDirectoryNolock(
          file->dh());
  if (!parent_dir) {
    block_fs_set_errno(EISDIR);
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

  // 同步从节点
  LOG(INFO) << "remove file success: " << file->file_name();
  block_fs_set_errno(0);
  return 0;
}

int FileHandle::unlink(const std::string &filename) {
  LOG(INFO) << "unlink file: " << filename;

  const DirectoryPtr &dir =
      FileStore::Instance()->dir_handle()->GetCreatedDirectory(filename);
  if (dir) {
    block_fs_set_errno(EISDIR);
    return -1;
  }

  std::string new_dirname;
  std::string new_filename;
  if (!TransformPath(filename, new_dirname, new_filename)) {
    return -1;
  }

  std::pair<DirectoryPtr, FilePtr> dirs;
  if (unlikely(!FindFile(new_dirname, new_filename, &dirs))) {
    return -1;
  }
  DirectoryPtr parent_dir = dirs.first;
  FilePtr curr_file = dirs.second;

  if (curr_file->LinkCount() > 0) {
    LOG(WARNING) << "file count not zero: " << curr_file->LinkCount();
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

  // 同步从节点
  LOG(INFO) << "remove file success: " << filename;
  block_fs_set_errno(0);
  return 0;
}

int FileHandle::mkstemp(char *template_str) { return -1; }

bool FileHandle::CheckFileExist(const std::string &path) {
  return (GetCreatedFile(path) == kEmptyFilePtr);
}

const OpenFilePtr &FileHandle::GetOpenFile(int32_t fd) {
  META_HANDLE_LOCK();
  return GetOpenFileNolock(fd);
}

const OpenFilePtr &FileHandle::GetOpenFileNolock(int32_t fd) {
  auto itor = open_files_.find(fd);
  if (unlikely(itor == open_files_.end())) {
    LOG(WARNING) << "fd not been opened: " << fd;
    /* ERROR: invalid file descriptor */
    block_fs_set_errno(EBADF);
    return kEmptyOpenFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFile(int32_t fh) {
  META_HANDLE_LOCK();
  return GetCreatedFileNoLock(fh);
}

const FilePtr &FileHandle::GetCreatedFileNoLock(int32_t fh) {
  auto itor = created_fhs_.find(fh);
  if (itor == created_fhs_.end()) {
    LOG(ERROR) << "file handle not exist: " << fh;
    return kEmptyFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFile(const std::string &filename) {
  LOG(INFO) << "get created file name: " << filename;
  std::string new_dirname;
  std::string new_filename;
  if (!TransformPath(filename, new_dirname, new_filename)) {
    return kEmptyFilePtr;
  }
  const DirectoryPtr &dir =
      FileStore::Instance()->dir_handle()->GetCreatedDirectory(new_dirname);
  if (unlikely(!dir)) {
    // block_fs_set_errno(ENOTDIR);
    return kEmptyFilePtr;
  }
  META_HANDLE_LOCK();
  FileNameKey key = std::make_pair(dir->dh(), new_filename);
  return GetCreatedFileNoLock(key);
}

const FilePtr &FileHandle::GetCreatedFileNoLock(const FileNameKey &key) {
  auto itor = created_files_.find(key);
  if (unlikely(itor == created_files_.end())) {
    LOG(WARNING) << "file not exist: " << key.second;
    /* ERROR: trying to open a file that does not exist without O_CREATE */
    block_fs_set_errno(ENOENT);
    return kEmptyFilePtr;
  }
  return itor->second;
}

const FilePtr &FileHandle::GetCreatedFileNolock(int32_t dh,
                                                const std::string &filename) {
  FileNameKey key = std::make_pair(dh, filename);
  auto itor = created_files_.find(key);
  if (unlikely(itor == created_files_.end())) {
    LOG(WARNING) << "file not exist: " << filename;
    return kEmptyFilePtr;
  }
  return itor->second;
}

void FileHandle::AddOpenFile(int32_t fd, const OpenFilePtr &file) noexcept {
  META_HANDLE_LOCK();
  open_files_[fd] = file;
}
void FileHandle::RemoveOpenFile(int32_t fd, const OpenFilePtr &file) noexcept {
  META_HANDLE_LOCK();
  open_files_.erase(fd);
}

static bool HasWriteFlag(const int32_t flags) {
  if (!(flags & O_RDWR) && !(flags & O_WRONLY)) {
    return false;
  }
  return true;
}

static bool HasTruncateFlag(const int32_t flags) {
  /* if O_TRUNC is set with RDWR or WRONLY, need to truncate file */
  return ((flags & O_TRUNC) && (flags & (O_RDWR | O_WRONLY)));
}

static bool VerifyOpenExistFileFlag(const int32_t flags) {
  /* trying to open a file that exists with O_CREATE and O_EXCL
   * if O_CREAT and O_EXCL are set, this is an error */
  if ((flags & O_CREAT) && (flags & O_EXCL)) {
    block_fs_set_errno(EEXIST);
    return false;
  }
  /* if O_DIRECTORY is set and fid is not a directory, error */
  if ((flags & O_DIRECTORY)) {
    block_fs_set_errno(ENOTDIR);
    return false;
  }
  return true;
}

static bool IsNeedCreateTempFile(const int32_t flags) {
  if (!(flags & O_TMPFILE)) {
    return false;
  }
  if (!HasWriteFlag(flags)) {
    LOG(ERROR) << "invalid tmp open flag without O_RDWR or O_WRONLY";
    errno = EINVAL;
    return false;
  }
  return true;
}

static bool IsNeedCreateNormalFile(const int32_t flags) {
  /* file does not exist, create file if O_CREAT is set */
  if (!(flags & O_CREAT)) {
    LOG(DEBUG) << "open file without create flag";
    return false;
  }
  return true;
}

int FileHandle::open(const std::string &filename, int32_t flags, mode_t mode) {
  int32_t ret;
  FilePtr file;
  if (IsNeedCreateTempFile(flags)) {
    // when create temp file, the filename is dirname
    file = CreateFile(filename, flags, true);
    if (!file) {
      return -1;
    }
  } else {
    if (FileStore::Instance()->dir_handle()->GetCreatedDirectory(filename)) {
      errno = EISDIR;
      return -1;
    }
    file = GetCreatedFile(filename);
    if (!file) {
      if (IsNeedCreateNormalFile(flags)) {
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
      if (HasTruncateFlag(flags)) {
        ret = file->ftruncate(0);
        if (ret < 0) {
          return -1;
        }
      }
    }
  }

  /* allocate a file fd for this new file */
  int32_t fd = FileStore::Instance()->fd_handle()->get_fd();
  if (fd < 0) {
    errno = ENFILE;
    return -1;
  }
  LOG(INFO) << "open file: " << filename << " fd: " << fd
            << " fh: " << file->fh();
  OpenFilePtr open_file = std::make_shared<OpenFile>(file);
  AddOpenFile(fd, open_file);
  open_file->set_open_fd(fd);

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

int FileHandle::close(int32_t fd) noexcept {
  META_HANDLE_LOCK();
  const OpenFilePtr &open_file = GetOpenFileNolock(fd);
  if (unlikely(!open_file)) {
    return -1;
  }
  const FilePtr &file = open_file->file();
  file->fsync();
  LOG(INFO) << "close file name: " << file->file_name() << " fd: " << fd;
  // 文件关闭的时候去掉文件锁
  file->set_locked(false);
  file->DecLinkCount();
  // TODO:临时文件或者文件已经被删除, 引用为0自动删除
  if (file->LinkCount() == 0) {
    if (file->is_temp()) {
      LOG(WARNING) << "cleanup temp file fh: " << file->fh();
      UnlinkFileNolock(file->fh());
    } else if (file->deleted()) {
      LOG(WARNING) << "clean deleted file fh: " << file->fh();
      UnlinkFileNolock(file->fh());
    }
  }
  // keep the reference of file pointer
  open_files_.erase(fd);
  FileStore::Instance()->fd_handle()->put_fd(fd);
  block_fs_set_errno(0);
  return 0;
}

int FileHandle::dup(int oldfd) {
  const OpenFilePtr &old_open_file = GetOpenFile(oldfd);
  if (!old_open_file) {
    // block_fs_set_errno(ENOENT);
    return -1;
  }
  const FilePtr &file = old_open_file->file();

  /* allocate a file fd for this new file */
  int32_t newfd = FileStore::Instance()->fd_handle()->get_fd();
  if (newfd < 0) {
    block_fs_set_errno(ENFILE);
    return -1;
  }
  LOG(INFO) << "dup oldfd: " << oldfd << " newfd: " << newfd
            << " fh: " << file->fh();
  OpenFilePtr new_open_file = std::make_shared<OpenFile>(file);
  AddOpenFile(newfd, new_open_file);
  new_open_file->set_open_fd(newfd);
  file->IncLinkCount();

  block_fs_set_errno(0);
  return newfd;
}

int FileHandle::fsync(int32_t fd) {
  const OpenFilePtr &open_file = GetOpenFile(fd);
  if (!open_file) {
    block_fs_set_errno(ENOENT);
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
  for (auto &kv : created_fhs_) {
    LOG(INFO) << "find fh: " << kv.first << " name: " << kv.second->file_name();
  }
}

void FileHandle::Dump(const std::string &file_name) noexcept {}

}