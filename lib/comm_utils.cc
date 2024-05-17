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

namespace udisk::blockfs {

// https://blog.csdn.net/butterfly5211314/article/details/84575883
// new_dirname = ::dirname(full_path.c_str());
// new_filename = ::basename(full_path.c_str());
std::string GetFileName(const std::string &path) {
  char ch = '/';
  size_t pos = path.rfind(ch);
  if (pos == std::string::npos)
    return path;
  else
    return path.substr(pos + 1);
}

std::string GetDirName(const std::string &path) {
  char ch = '/';
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

bool RunAsAdmin() {
#ifdef WIN32
  return IsUserAnAdmin();  // true, is admin
#else
  return (::geteuid() == 0);                    // true, is root
#endif
}

}