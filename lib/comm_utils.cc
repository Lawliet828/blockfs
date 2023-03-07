#include "comm_utils.h"

#include <dirent.h>
#include <limits.h>
#include <mntent.h>
#include <signal.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

#include "logging.h"

namespace udisk {
namespace blockfs {

// https://blog.csdn.net/butterfly5211314/article/details/84575883
// new_dirname = ::dirname(full_path.c_str());
// new_filename = ::basename(full_path.c_str());
std::string GetFileName(const std::string &path) {
  char ch = '/';
#ifdef _WIN32_
  ch = '\\';
#endif
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

std::string GetDirName(const std::string &path) {
  char ch = '/';
#ifdef _WIN32_
  ch = '\\';
#endif
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(0, pos + 1);
}

std::string GetParentDirName(const std::string &path) {
  std::string parent = path;
  if (parent == "/") {
    return parent;
  }
  if (parent[parent.size() - 1] == '/') {
    parent.erase(parent.size() - 1);
  }
  return GetDirName(parent);
}

/* allow coredump after setuid() in Linux 2.4.x */
bool SetCoreDump(bool on) {
  int opt = on ? 1 : 0;
  int value = 0;
  if (::prctl(PR_GET_DUMPABLE, &value) < 0) {
    LOG(ERROR) << "prctl get dumpable failed, errno: " << errno;
    return false;
  }
  if (value == opt) {
    LOG(DEBUG) << "process is already dumpable";
    return true;
  }
  /* Set Linux DUMPABLE flag */
  if (::prctl(PR_SET_DUMPABLE, opt, 0, 0, 0) < 0) {
    LOG(ERROR) << "prctl set dumpable failed, errno: " << errno;
    return false;
  }

  /* Make sure coredumps are not limited */
  struct rlimit rlim;
  if (::getrlimit(RLIMIT_CORE, &rlim) < 0) {
    LOG(ERROR) << "get coredump limit, errno: " << errno;
    return false;
  }
  rlim.rlim_cur = rlim.rlim_max;
  if (::setrlimit(RLIMIT_CORE, &rlim) < 0) {
    LOG(ERROR) << "set coredump limit, errno: " << errno;
    return false;
  }
  LOG(DEBUG) << "coredump rlim_cur: " << rlim.rlim_cur
             << " max rlim_max: " << rlim.rlim_max;
  return true;
}

void DebugHexBuffer(void *buffer, uint32_t len) {
  if (!buffer || 0 == len) {
    LOG(INFO) << "????";
    return;
  }

  const uint32_t bytes_per_line = 16;
  uint8_t *p = (uint8_t *)buffer;
  char hex_str[3 * bytes_per_line + 1] = {0};

  LOG(INFO) << "-----------------begin-------------------\n";
  for (uint32_t i = 0; i < len; ++i) {
    int idx = 3 * (i % bytes_per_line);
    if (0 == idx) {
      ::memset(hex_str, 0, sizeof(hex_str));
    }
#ifdef WIN32
    sprintf_s(&hex_str[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#else
    snprintf(&hex_str[idx], 4, "%02x ", p[i]);  // buff长度要多传入1个字节
#endif

    // 以16个字节为一行，进行打印
    if (0 == ((i + 1) % bytes_per_line)) {
      LOG(INFO) << hex_str;
    }
  }

  // 打印最后一行未满16个字节的内容
  if (0 != (len % bytes_per_line)) {
    LOG(INFO) << hex_str;
  }
  LOG(INFO) << "-----------------end-------------------\n";
}

bool RunAsAdmin() {
#ifdef WIN32
  return IsUserAnAdmin();  // true, is admin
#else
  return (::geteuid() == 0);                    // true, is root
#endif
}

// /proc/pid/stat字段定义
struct PidStat {
  int64_t pid;
  char comm[256];
  char state;
  int64_t ppid;
  int64_t pgrp;
  int64_t session;
  int64_t tty_nr;
  int64_t tpgid;
  int64_t flags;
  int64_t minflt;
  int64_t cminflt;
  int64_t majflt;
  int64_t cmajflt;
  int64_t utime;
  int64_t stime;
  int64_t cutime;
  int64_t cstime;
  // ...
};

std::string PidName(pid_t pid) {
  std::string name = "/proc/" + std::to_string(pid) + "/stat";
  FILE *pid_stat = ::fopen(name.c_str(), "r");
  if (!pid_stat) {
    return "";
  }

  PidStat result;
  int ret = ::fscanf(
      pid_stat,
      "%ld %s %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
      &result.pid, result.comm, &result.state, &result.ppid, &result.pgrp,
      &result.session, &result.tty_nr, &result.tpgid, &result.flags,
      &result.minflt, &result.cminflt, &result.majflt, &result.cmajflt,
      &result.utime, &result.stime, &result.cutime, &result.cstime);

  ::fclose(pid_stat);

  if (ret <= 0) {
    return "";
  }
  name = result.comm;
  name.erase(0, name.find_first_not_of("("));
  name.erase(name.find_last_not_of(")") + 1);
  return name;
}

// https://github.com/PeaJune/pidof/blob/master/pidof.cpp
bool PidOF(const std::string &name, std::vector<pid_t> &vec) {
  DIR *dir = ::opendir("/proc");
  if (!dir) {
    LOG(ERROR) << "failed to open proc directory";
    return false;
  }
  pid_t pid;
  struct dirent *de = nullptr;
  while ((de = ::readdir(dir)) != nullptr) {
    LOG(DEBUG) << "de->d_name: " << de->d_name;
    if ((pid = ::atoi(de->d_name)) == 0) continue;
    if (pid == ::getpid()) continue;
    if (name == PidName(pid)) {
      vec.push_back(pid);
    }
  }
  ::closedir(dir);
  return true;
}

bool KillPid(pid_t pid) {
  int deadcnt = 20;
  struct stat st;

  std::string name = "/proc/" + std::to_string(pid) + "/stat";
  ::kill(pid, SIGTERM);
  while (deadcnt--) {
    ::usleep(100 * 1000);
    if ((::stat(name.c_str(), &st) > -1) && (st.st_mode & S_IFREG)) {
      ::kill(pid, SIGKILL);
      int status;
      pid_t result;

      /* wait不能使用在while中, 因为wait是阻塞的,比如当某子进程一直不停止,
       * 它将一直阻塞, 从而影响了父进程.于是使用waitpid, options设置为WNOHANG,
       * 可以在非阻塞情况下处理多个退出子进程
       */
      do {
        result = ::waitpid(pid, &status, 0);
      } while ((result < 0) && (errno == EINTR));

      if (result == -1) {
        LOG(ERROR) << "failed to wait a process with pid: " << pid;
        continue;
      }
      if (WIFEXITED(status)) {
        LOG(INFO) << "process " << pid
                  << " normal exit, status: " << WEXITSTATUS(status);
        // return WEXITSTATUS(status);
        return true;
      } else if (WIFSIGNALED(status)) {
        LOG(ERROR) << "process: " << pid
                   << " was killed by signal: " << WTERMSIG(status);
      } else if (WIFSTOPPED(status)) {
        LOG(ERROR) << "process: " << pid
                   << " was stop by signal: " << WSTOPSIG(status);
      } else if (WIFCONTINUED(status)) {
        LOG(ERROR) << "process: " << pid << " was continued by signal SIGCONT";
      } else {
        LOG(ERROR) << "process: " << pid << " has unknown wait status";
      }
    } else {
      break;
    }
  }
  return deadcnt > 0;
}

// https://www.cnblogs.com/leeming0222/articles/3994125.html
bool KillAll(const std::string &name) {
  std::vector<pid_t> pids;
  if (!PidOF(name, pids)) {
    return false;
  }
  for (auto pid : pids) {
    LOG(INFO) << name << " kill old pid: " << pid;
    if (!KillPid(pid)) {
      return false;
    }
  }
  pids.clear();
  if (!PidOF(name, pids)) {
    return false;
  }
  return pids.empty();
}

// https://blog.csdn.net/dosthing/article/details/81276208
// https://blog.csdn.net/luckywang1103/article/details/49822333
bool UnmountPath(const std::string &path) {
  if (path.empty() || path == "/") {
    return true;
  }
  std::string mount_path = path;
  if (mount_path[mount_path.size() - 1] == '/') {
    mount_path.erase(mount_path.size() - 1);
  }
  struct mntent *ent = nullptr;
  char mntent_buffer[5 * FILENAME_MAX];
  struct mntent temp_ent;
  /* open '/proc/mounts' to load mount points */
  FILE *proc_mount = ::fopen("/proc/mounts", "re");
  if (proc_mount == nullptr) return false;
  while ((ent = ::getmntent_r(proc_mount, &temp_ent, mntent_buffer,
                              sizeof(mntent_buffer))) != nullptr) {
    std::string mount_point = ent->mnt_dir;
    if (mount_point == mount_path) {
      LOG(INFO) << "The mount device: " << ent->mnt_fsname
                << " the mount point: " << ent->mnt_dir
                << " umount target mount point: " << mount_path;
      if (::umount2(ent->mnt_dir, MNT_FORCE) < 0) {
        return false;
      }
    }
  }
  ::fclose(proc_mount);
  return true;
}

}  // namespace blockfs
}  // namespace udisk