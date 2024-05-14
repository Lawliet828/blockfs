// Copyright (c) 2021 UCloud All rights reserved.

#include "injection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <mutex>
#include <thread>
#include <unordered_map>

using namespace udisk::blockfs;

namespace udisk {
namespace blockfs {

struct InjectionPoint {
  InjectionPoint() = default;
  InjectionPoint(const std::string &key, uint32_t count)
      : key_(key), count_(count) {}
  ~InjectionPoint() {}
  std::string key_;
  uint32_t count_;
  // bool sucide_; // sucide right now
};

class FaultInjection {
 public:
  FaultInjection() = default;
  ~FaultInjection() = default;

  void KillMyself() { ::raise(SIGKILL); }
  void InjectOne(const InjectionPoint &point) {
    std::lock_guard<std::mutex> lock(mutex);
    if (point.key_ == "kill_now") {
      LOG(WARNING) << "stub kill right now";
      KillMyself();
      return;
    }
    LOG(WARNING) << "inser key: " << point.key_ << " count: " << point.count_;
    key_map_[point.key_] = std::move(point);
  }
  void MatchStub(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = key_map_.find(key);
    if (it == key_map_.end()) {
      return;
    }
    LOG(WARNING) << "stub kill at key: " << key;
    KillMyself();
  }

 private:
  std::mutex mutex;
  std::unordered_map<std::string, InjectionPoint> key_map_;
};

static FaultInjection g_inject;

int CreateSocket(const int domain, const int type, const int protocol) {
  int fd = ::socket(domain, type | SOCK_NONBLOCK, protocol);
  if (fd == -1) {
    LOG(ERROR) << "failed to open socket:" << strerror(errno);
    return fd;
  }
  int flags;
  if ((flags = ::fcntl(fd, F_GETFD, NULL)) < 0) {
    return false;
  }
  if (!(flags & FD_CLOEXEC)) {
    if (::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
      return false;
    }
  }
  return fd;
}

void CloseSocket(const int fd) {
  if (fd > 0) {
#ifdef WIN32
    (void)closesocket(fd);
#else
    ::close(fd);
#endif
  }
}

int CreateUnixSocket() { return CreateSocket(AF_UNIX, SOCK_STREAM, 0); }

bool SetReuseAddr(const int fd, bool on) {
  int flag = on;
#if defined(SO_REUSEADDR) && !defined(_WIN32)
  /* REUSEADDR on Unix means, "don't hang on to this address after the
   * listener is closed."  On Windows, though, it means "don't keep other
   * processes from binding to this address while we're using it. */
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag,
                   (int)sizeof(flag)) < 0) {
    return false;
  }
#endif
  return true;
}

/* create a UNIX domain stream socket */
int CreateUnixServer(const std::string &srv_path, const uint32_t backlog,
                     const uint32_t access_mask) {
  struct stat unix_stat;
  mode_t mode;
  int old_umask;
  struct sockaddr_un s_un;

  memset(&s_un, 0, sizeof(struct sockaddr_un));
  s_un.sun_family = AF_UNIX;

  int fd = CreateUnixSocket();
  if (fd < 0) {
    return -1;
  }

  SetReuseAddr(fd, true);

  /* Clean up a previous socket file if we left it around */
  if (::lstat(srv_path.c_str(), &unix_stat) == 0) {
    /* in case it already exists */
    if (S_ISSOCK(unix_stat.st_mode)) {
      ::unlink(srv_path.c_str());
    }
  }

  mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (::chmod((char *)srv_path.c_str(), mode) == -1) {
    LOG(ERROR) << "chmod access failed for " << srv_path;
  }

  ::memcpy(s_un.sun_path, srv_path.c_str(),
           std::min(sizeof(s_un.sun_path) - 1, srv_path.size()));
  // int len = offsetof(struct sockaddr_un, sun_path) + strlen(s_un.sun_path);

  old_umask = ::umask(~(access_mask & 0777));

  /* bind the name to the descriptor */
  if (::bind(fd, (struct sockaddr *)&s_un, sizeof(s_un)) < 0) {
    ::umask(old_umask);
    LOG(ERROR) << "bind failed for " << s_un.sun_path
               << " errno: " << strerror(errno);
    CloseSocket(fd);
    return -1;
  }
  ::umask(old_umask);

  if (::listen(fd, backlog) < 0) {
    /* tell kernel we're a server */
    LOG(ERROR) << "listen call failed errno :" << strerror(errno);
    CloseSocket(fd);
    return -1;
  }
  return fd;
}

void DebugSocket(const int fd) {
  int ret;
  char ip[64];
  struct sockaddr_in *sin = NULL;
  struct sockaddr_in6 *sin6 = NULL;
  struct sockaddr_un *cun = NULL;
  struct sockaddr_storage cliAddr;
  socklen_t addrlen = sizeof(cliAddr);

  ::memset(ip, 0, sizeof(ip));
  ::memset(&cliAddr, 0, sizeof(cliAddr));

  ret = ::getsockname(fd, (struct sockaddr *)&cliAddr, &addrlen);
  if (ret < 0) {
    LOG(ERROR) << "conn getsockname failed,";
    return;
  }
  if (cliAddr.ss_family == AF_UNIX) {
    cun = (struct sockaddr_un *)(&cliAddr);
    LOG(INFO) << "new conn from unix path: " << cun->sun_path;
  } else if (cliAddr.ss_family == AF_INET) {
    sin = (struct sockaddr_in *)(&cliAddr);
    /* inet_ntoa(sin->sin_addr) */
    ::inet_ntop(cliAddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
    LOG(INFO) << "new conn from ipv4: " << ip
              << ", port: " << ntohs(sin->sin_port);
  } else if (cliAddr.ss_family == AF_INET6) {
    sin6 = (struct sockaddr_in6 *)(&cliAddr);
    ::inet_ntop(cliAddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
    LOG(INFO) << "new conn from ipv6: " << ip
              << ", port: " << ntohs(sin6->sin6_port);
  }

  ::memset(ip, 0, sizeof(ip));
  ::memset(&cliAddr, 0, sizeof(cliAddr));
  ret = ::getpeername(fd, (struct sockaddr *)&cliAddr, &addrlen);
  if (ret < 0) {
    // LOG(ERROR) << "Conn getpeername failed";
    return;
  }

  if (cliAddr.ss_family == AF_UNIX) {
    cun = (struct sockaddr_un *)(&cliAddr);
    LOG(INFO) << "new conn from unix path: " << cun->sun_path;
  } else if (cliAddr.ss_family == AF_INET) {
    sin = (struct sockaddr_in *)(&cliAddr);
    /* inet_ntoa(sin->sin_addr) */
    ::inet_ntop(cliAddr.ss_family, &sin->sin_addr, ip, sizeof(ip));
    LOG(INFO) << "new conn from ipv4: " << ip
              << ", port: " << ntohs(sin->sin_port);
  } else if (cliAddr.ss_family == AF_INET6) {
    sin6 = (struct sockaddr_in6 *)(&cliAddr);
    ::inet_ntop(cliAddr.ss_family, &sin6->sin6_addr, ip, sizeof(ip));
    LOG(INFO) << "new conn from ipv6: " << ip
              << ", port: " << ntohs(sin6->sin6_port);
  }
}

void ServerPoll(int sfd) {
  struct pollfd fds[512];
  fds[0].fd = sfd;
  fds[0].events = POLLIN | POLLERR;
  fds[0].revents = 0;
  int conncount = 0;
  std::unordered_map<int, std::string> mapdata;

  while (true) {
    int ret = ::poll(fds, conncount + 1, -1);
    if (ret < 0) {
      LOG(FATAL) << "poll errno: " << errno;
      ::exit(1);
    }

    for (int i = 0; i < conncount + 1; i++) {
      // 客户端关闭，或者错误。错误的原因是由于客户端关闭导致的，这里一并处理
      if ((fds[i].revents & POLLRDHUP) || (fds[i].revents & POLLERR)) {
        int fd = fds[i].fd;
        fds[i] = fds[conncount];
        i--;
        conncount--;
        mapdata.erase(fd);
        CloseSocket(fd);
        LOG(WARNING) << "delete connection: " << fd;
      }
      // 新的连接
      else if ((fds[i].fd == sfd) && (fds[i].revents & POLLIN)) {
        struct sockaddr_in client;
        socklen_t lenaddr = sizeof(client);
        int conn = -1;

        if (-1 ==
            (conn = ::accept(sfd, (struct sockaddr *)&client, &lenaddr))) {
          LOG(FATAL) << "accept errno: " << errno;
          ::exit(1);
        }
        DebugSocket(conn);
        conncount++;
        int flags;
        if ((flags = ::fcntl(conn, F_GETFL, NULL)) < 0) {
          LOG(ERROR) << "fail to get nonblocking flag for socket: " << conn;
          exit(1);
        }
        if (::fcntl(conn, F_SETFL, flags | O_NONBLOCK | O_RDWR) < 0) {
          LOG(ERROR) << "fail to set nonblocking for socket: " << conn;
          exit(1);
        }
        fds[conncount].fd = conn;
        fds[conncount].events = POLLIN | POLLRDHUP | POLLERR;
        fds[conncount].revents = 0;
      }
      // 有可读数据
      else if (fds[i].revents & POLLIN) {
        char buf[1024] = {0};

        int lenrecv = ::recv(fds[i].fd, buf, 1024 - 1, 0);
        if (lenrecv > 0) {
          block_fs_stub_inject(buf, 1);

          ::memset(buf, 0, sizeof(buf));
          ::memcpy(buf, "success", sizeof("success"));

          mapdata[fds[i].fd] = buf;
          fds[i].events &= (~POLLIN);
          fds[i].events |= POLLOUT;

        } else if (lenrecv == 0) {
          LOG(WARNING) << "client " << fds[i].fd << " exist";
        } else {
          LOG(WARNING) << "client " << fds[i].fd << " recv errno: " << errno;
          ::exit(1);
        }
      }
      // 可写数据
      else if (fds[i].revents & POLLOUT) {
        if (::send(fds[i].fd, mapdata[fds[i].fd].c_str(),
                   mapdata[fds[i].fd].size(), 0) < 0) {
          if (ECONNRESET == errno) {
            continue;
          }
          LOG(WARNING) << "client " << fds[i].fd << " send errno: " << errno;
          ::exit(1);
        }
        fds[i].events &= (~POLLOUT);
        fds[i].events |= POLLIN;
      }
    }
  }
}

std::thread *g_thread;
void block_fs_stub_init(block_fs_config_info *info) {
  if (info->enable_agent_) {
    int sfd = CreateUnixServer(info->agent_server_, 5, 755);
    if (sfd < 0) {
      ::exit(1);
    }
    g_thread = new std::thread(ServerPoll, sfd);
  }
}

// by outer pb
void block_fs_stub_inject(const std::string &key, uint32_t count) {
  g_inject.InjectOne(InjectionPoint(key, count));
}

}  // namespace blockfs
}  // namespace udisk
