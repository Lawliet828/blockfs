// Copyright (c) 2020 UCloud All rights reserved.
#include "file_lock.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"

using namespace udisk::blockfs;

PosixFileLock::~PosixFileLock() {
  if (fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool PosixFileLock::lock(bool create) {
  int32_t flag = 0;
  if (create) {
    flag = O_RDWR | O_CREAT | O_CLOEXEC;
  } else {
    flag = O_RDWR | O_CLOEXEC;
  }
  fd_ = ::open(file_name_.c_str(), flag, 0644);
  if (fd_ < 0) {
    LOG(ERROR) << "failed to open " << file_name_ << ", errno:" << errno;
    return false;
  }

  struct flock l;
  memset(&l, 0, sizeof(l));
  l.l_type = F_WRLCK;
  l.l_whence = SEEK_SET;
  l.l_start = 0;
  l.l_len = 0;
  int32_t ret = ::fcntl(fd_, F_SETLK, &l);
  if (ret < 0) {
    if (errno == EAGAIN || errno == EACCES) {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno
                 << ", maybe another process locked it";
    } else {
      LOG(ERROR) << "failed to lock " << file_name_ << ", errno: " << errno;
    }
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  ::ftruncate(fd_, 0);
  char buf[16] = {0};
  ::snprintf(buf, sizeof(buf), "%ld", (long)getpid());
  ::write(fd_, buf, ::strlen(buf) + 1);
  LOG(DEBUG) << "file lock " << file_name_ << " success";
  return true;
}

bool PosixFileLock::unlock() {
  if (fd_ < 0) {
    LOG(ERROR) << "file name: " << file_name_ << " has not been opened";
    return false;
  }
  struct flock f;
  ::memset(&f, 0, sizeof(f));
  f.l_type = F_UNLCK;
  f.l_whence = SEEK_SET;
  f.l_start = 0;
  f.l_len = 0;  // Lock/unlock entire file
  int32_t ret = ::fcntl(fd_, F_SETLK, &f);
  if (ret < 0) {
    LOG(ERROR) << "failed to unlock " << file_name_ << ", errno: " << errno;
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  LOG(INFO) << "unlock " << file_name_ << " success";
  return true;
}
