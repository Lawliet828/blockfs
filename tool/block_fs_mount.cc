#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for

#include "block_fs.h"
#include "file_store_udisk.h"
#include "logging.h"

static void HelpInfo() {
  LOG(INFO) << "Build time    : " << __DATE__ << " " << __TIME__;
  LOG(INFO) << "Run options:";
  LOG(INFO) << " -c, --config   /data/blockfs/conf/bfs.cnf";
  LOG(INFO) << " -d, --device   /dev/vdc";
  LOG(INFO) << " -m, --mountpoint  /data/mysql/bfs";
  LOG(INFO) << " -v, --version  Print the version.";
  LOG(INFO) << " -h, --help     Print help info.";
}

// 参考lightttpd和nginx
// https://blog.csdn.net/weixin_40021744/article/details/105216611
// https://blog.csdn.net/fall221/article/details/45420197
// http://blog.chinaunix.net/uid-29482215-id-4123005.html

/* if we want to ensure our ability to dump core, don't chdir to / */
void daemonize(bool chdir = false, bool close = false) {
  // 屏蔽一些有关控制终端操作的信号,
  // 防止守护进程没有正常运作之前控制终端受到干扰退出或挂起
  signal(SIGTTOU, SIG_IGN);  // 忽略后台进程写控制终端信号
  signal(SIGTTIN, SIG_IGN);  // 忽略后台进程读控制终端信号
  signal(SIGTSTP, SIG_IGN);  // 忽略终端挂起信号

  /* 从普通进程转换为守护进程
   * 步骤1: 后台运行
   * 操作1: 脱离控制终端->调用fork->终止父进程->子进程被init收养
   */
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    LOG(ERROR) << "fork son process failed, exit";
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    // 为了子进程完成fuse的调用后再返回,此处停留1s
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(EXIT_SUCCESS);
  }

  /* 步骤2: 脱离控制终端,登陆会话和进程组
   * 操作2: 使用setsid创建新会话,成为新会话的首进程,则与原来的
   *        登陆会话和进程组自动脱离,从而脱离控制终端.
   *       一步的fork保证了子进程不可能是一个会话的首进程,
   *        这是调用setsid的必要条件.
   */
  if (setsid() < 0) {
    LOG(ERROR) << "setsid process failed, exit";
    exit(EXIT_FAILURE);
  }

  /* set files mask */
  umask(0);

  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* 步骤3: 孙子进程被托管给init进程
   *        上面已经完成了大部分工作, 但是有的系统上,当会话首进程打开
   *        一个尚未与任何会话关联的终端时,该设备自动作为控制终端分配给该会话
   *        为避免该情况,我们再次fork进程,于是新进程不再是会话首进程
   *        会话首进程退出时可能会给所有会话内的进程发送SIGHUP，而该
   *        信号默认是结束进程,故需要忽略该信号来防止孙子进程意外结束。
   */
  pid = fork();
  if (pid < 0) {
    LOG(ERROR) << "fork grandson process failed, exit";
    exit(EXIT_FAILURE);
  }
  if (pid > 0) exit(EXIT_SUCCESS);

  /* 步骤4: 改变工作目录到根目录
   *       进程活动时,其工作目录所在的文件系统不能被umount
   */
  if (chdir) {
    char path_dir[0x100] = {0};
    (void)getcwd(path_dir, 0x100);
    if (::chdir(path_dir) != 0) {
      LOG(ERROR) << "daemon process chdir failed, exit";
      exit(EXIT_FAILURE);
    }
  }

  /* 步骤5: 重定向标准输入输出到/dev/null
   *        但是不关闭012的fd避免被分配
   */
  if (close) {
    int fd = ::open("/dev/null", O_RDWR);
    if (fd < 0) {
      LOG(ERROR) << "daemon process open /dev/null failed, exit";
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
      LOG(ERROR) << "daemon process dup2 stdin failed, exit";
      exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
      LOG(ERROR) << "daemon process dup2 stdout failed, exit";
      exit(EXIT_FAILURE);
    }
    // if (dup2(fd, STDERR_FILENO) < 0) {
    //   LOG(ERROR) << "daemon process dup2 stderr failed, exit";
    //   exit(EXIT_FAILURE);
    // }

    if (fd > STDERR_FILENO) {
      if (::close(fd) < 0) {
        LOG(ERROR) << "daemon process close fd failed, exit";
        exit(EXIT_FAILURE);
      }
    }
  }
}

void ToolLogPreInit(const std::string &name, const std::string &dirname) {
  static int log_fd = -1;

  std::string filename = dirname;
  if (filename[filename.size() - 1] != '/') {
    filename += "/";
  }

  std::string format = name + ".%Y%m%d-%H%M%S";

  char timebuf[32];
  struct tm tm;
  time_t now = ::time(NULL);
  ::localtime_r(&now, &tm);
  ::strftime(timebuf, sizeof timebuf, format.c_str(), &tm);

  filename += timebuf;
  filename += ".log";

  log_fd = ::open(filename.c_str(), O_CREAT | O_RDWR | O_APPEND | O_SYNC, 0755);
  if (log_fd < 0) {
    LOG(ERROR) << "failed to open log file: " << filename
               << " errno: " << errno;
    std::exit(1);
  }
  ::dup2(log_fd, STDOUT_FILENO);
  ::dup2(log_fd, STDERR_FILENO);
}

int main(int argc, char *argv[]) {
  const char *config_path = "/data/blockfs/conf/bfs.cnf";
  std::string device;
  std::string mountpoint;
  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"config", required_argument, nullptr, 'c'},
        {"device", required_argument, nullptr, 'd'},
        {"mountpoint", required_argument, nullptr, 'm'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = ::getopt_long(argc, argv, "c:d:m:vh?", longOpts, &optIndex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c': {
        config_path = optarg;
      } break;
      case 'd': {
        device = std::string(optarg);
      } break;
      case 'm': {
        mountpoint = std::string(optarg);
      } break;
      case 'v':
      case 'h':
      default: {
        HelpInfo();
        std::exit(0);
      }
    }
  }
  if (!config_path) {
    printf("config path is empty\n");
    HelpInfo();
    std::exit(1);
  }
  if (device.empty() || mountpoint.empty()) {
    printf("device or mountpoint is empty\n");
    HelpInfo();
    std::exit(1);
  }

  if (FileSystem::Instance()->MountFileSystem(config_path) < 0) {
    return -1;
  }

  bfs_config_info *conf = FileSystem::Instance()->mount_config();
  conf->fuse_mount_point = mountpoint;

  ToolLogPreInit("block_fs_mount", conf->log_path_);

  // daemonize(true, false);

  block_fs_fuse_mount(FileSystem::Instance()->mount_config());
  return 0;
}