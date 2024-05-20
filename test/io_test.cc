#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <assert.h>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <fcntl.h>
#include <libaio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

static const uint32_t SECTOR_SIZE = 512U;
static const uint32_t PAGE_ALIGN_SIZE = 4 * 1024U;

class IoTest;
void help();

enum opType {
  O_CMD_READ = 0,
  O_CMD_WRITE = 1,
};

struct OnceIO {
  opType optype = O_CMD_READ;
  // 单次IO的起始扇区Index
  uint64_t sector = 0;
  // 单次IO的操作扇区数
  uint32_t sec_num = 0;
  void *data = NULL;
};

void aio_cb(evutil_socket_t fd, short event, void *ptr);
void run_test(IoTest *tester);

struct iocb *new_iocb() { return (struct iocb *)malloc(sizeof(struct iocb)); }
void free_iocb(struct iocb *p_iocb) { free(p_iocb); }

class IoTest {
 public:
  ~IoTest() {}

  void init_aio() {
    aio_efd_ = eventfd(0, 0);
    memset(&aio_ctx_, 0, sizeof(aio_ctx_));
    io_setup(1024, &aio_ctx_);
    aio_event_ = event_new(base_, aio_efd_, EV_READ | EV_WRITE | EV_PERSIST,
                           aio_cb, reinterpret_cast<void *>(this));
    event_add(aio_event_, NULL);
  }

  void free_event() {
    if (aio_event_ != NULL) {
      event_free(aio_event_);
      aio_event_ = NULL;
    }
    if (aio_ctx_ != 0) {
      io_destroy(aio_ctx_);
      aio_ctx_ = 0;
    }
    if (aio_efd_ != 0) {
      close(aio_efd_);
      aio_efd_ = 0;
    }
    if (timer_event_ != NULL) {
      event_free(timer_event_);
      timer_event_ = NULL;
    }
  }

  struct event_base *base_ = NULL;
  struct event *aio_event_ = NULL;
  struct event *timer_event_ = NULL;
  int aio_efd_ = 0;  // aio event fd
  io_context_t aio_ctx_;
  int parallel_ = 100;
  int job_num_ = INT_MAX;
  int job_do_ = 0;
  int fd_ = 0;
  int running_num_ = 0;
  std::string category_;
  std::string file_;
  // 文件的扇区总数
  uint64_t total_sector_num_ = 0;
  // 命令行指定的每次读写请求的扇区数
  int op_sector_num_ = 1;
  uint64_t magic_number_ = 42;

  uint64_t fail_num_ = 0;
  bool finish_ = false;
  bool has_trigger_ = false;
  std::vector<OnceIO *> io_list_;
  std::string io_list_file_;
  bool check_ = false;
  bool write_ = false;
  bool randcheck_ = false;
  bool randwrite_ = false;
  bool rw_ = false;
  bool randrw_ = false;
  bool debug_ = false;
};

std::string get_time() {
  time_t timep;
  time(&timep);
  char tmp[64];
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
  return tmp;
}

int SubmitIoRequest(IoTest *tester, OnceIO *onceIO) {
  void *data = onceIO->data;
  size_t data_size = onceIO->sec_num * SECTOR_SIZE;
  uint64_t offset = onceIO->sector * SECTOR_SIZE;

  struct iocb *iocb = new_iocb();

  if (onceIO->optype == O_CMD_WRITE) {
    io_prep_pwrite(iocb, tester->fd_, data, data_size, offset);
  } else {
    io_prep_pread(iocb, tester->fd_, data, data_size, offset);
  }

  iocb->data = reinterpret_cast<void *>(onceIO);
  io_set_eventfd(iocb, tester->aio_efd_);
  int ret = io_submit(tester->aio_ctx_, 1, &iocb);
  if (ret < 0) {
    free_iocb(iocb);
    return ret;
  }

  return 0;
}

void compare_data(void *expectation, void *reality, size_t data_size) {
  uint32_t offset = 0, offset2 = 0;
  uint32_t secnum = data_size / SECTOR_SIZE;
  for (uint32_t i = 0; i < secnum; ++i) {
    uint32_t compare_offset = i * SECTOR_SIZE;
    if (memcmp(reinterpret_cast<char *>(expectation) + compare_offset,
               reinterpret_cast<char *>(reality) + compare_offset,
               SECTOR_SIZE) != 0) {
      fprintf(stderr, "error sector offset: %d\n", i);
      offset = compare_offset;
      offset2 = compare_offset;
      for (uint32_t j = 0; j < SECTOR_SIZE / 8; ++j) {
        fprintf(stderr, "%#08lx ",
                *reinterpret_cast<uint64_t *>(
                    reinterpret_cast<char *>(expectation) + offset));
        offset += 8;
      }
      fprintf(stderr, "\n");
      for (uint32_t j = 0; j < SECTOR_SIZE / 8; ++j) {
        fprintf(stderr, "%#08lx ",
                *reinterpret_cast<uint64_t *>(
                    reinterpret_cast<char *>(reality) + offset2));
        offset2 += 8;
      }
      fprintf(stderr, "\n");
      fprintf(stderr, "\n");
    }
  }
}

bool check_read_content(OnceIO *onceIO, IoTest *tester) {
  if (!onceIO || !onceIO->data) {
    return false;
  }

  size_t data_size = onceIO->sec_num * SECTOR_SIZE;
  char data[data_size];
  memset(data, 0, sizeof(data));

  uint32_t offset = 0;
  for (uint64_t sector_no = onceIO->sector;
       sector_no < onceIO->sector + onceIO->sec_num; ++sector_no) {
    uint64_t info = sector_no + tester->magic_number_;
    for (uint32_t j = 0; j < SECTOR_SIZE / 8; ++j) {
      memcpy(reinterpret_cast<char *>(data) + offset, &info, sizeof(info));
      offset += 8;
    }
  }

  if (memcmp(data, onceIO->data, data_size) != 0) {
    fprintf(stderr, "check read content error\n");
    compare_data(data, onceIO->data, data_size);
    return false;
  }

  return true;
}

bool is_read_content_empty(OnceIO *onceIO) {
  if (!onceIO || !onceIO->data) return false;

  uint32_t data_size = onceIO->sec_num * SECTOR_SIZE;
  void *data = malloc(data_size);
  assert(data != 0);
  memset(data, 0, data_size);

  if (memcmp(data, onceIO->data, data_size) != 0) {
    fprintf(stderr, "%s read content is not empty error\n", get_time().c_str());
    compare_data(data, onceIO->data, data_size);
    free(data);
    return false;
  }

  free(data);
  return true;
}

void aio_cb(evutil_socket_t fd, short event, void *ptr) {
  IoTest *tester = reinterpret_cast<IoTest *>(ptr);
  if (event & EV_READ) {
    static const uint64_t EVENT_COUNT = 1024;
    struct io_event events[EVENT_COUNT];

    uint64_t finished_aio;
    ssize_t ret = read(tester->aio_efd_, &finished_aio, sizeof(uint64_t));
    assert(ret == sizeof(uint64_t));

    static timespec timeout = {0, 0};
    while (1) {
      uint64_t iRecvCount = std::min(finished_aio, EVENT_COUNT);
      int num_events =
          io_getevents(tester->aio_ctx_, 0, iRecvCount, events, &timeout);
      for (int i = 0; i < num_events; i++) {
        struct iocb *iocb = events[i].obj;
        // free_iocb(iocb);
        assert(tester->running_num_ > 0);
        --tester->running_num_;
        OnceIO *onceIO = (OnceIO *)iocb->data;
        if (iocb->aio_lio_opcode == IO_CMD_PREAD) {
          if (events[i].res != onceIO->sec_num * SECTOR_SIZE) {
            fprintf(stderr,
                    "%s read fail, read size: %ld, expected:%u, start_num:%lu, "
                    "sec_num:%u\n",
                    get_time().c_str(), events[i].res,
                    onceIO->sec_num * SECTOR_SIZE, onceIO->sector,
                    onceIO->sec_num);
            tester->fail_num_++;
          } else {
            bool check = check_read_content(onceIO, tester);
            if (!check) {
              tester->fail_num_++;
              fprintf(stderr, "%s read error, start_num:%lu, sec_num:%u\n",
                      get_time().c_str(), onceIO->sector, onceIO->sec_num);
              abort();
            } else {
              if (tester->debug_)
                fprintf(stderr, "read ok, start_num:%lu, sec_num:%u\n",
                        onceIO->sector, onceIO->sec_num);
            }
          }
        } else if (iocb->aio_lio_opcode == IO_CMD_PWRITE) {
          if (events[i].res != onceIO->sec_num * SECTOR_SIZE) {
            fprintf(stderr,
                    "%s write fail, size: %ld, expected: %u\n, start_num: %lu, "
                    "sec_num:%u\n",
                    get_time().c_str(), events[i].res,
                    onceIO->sec_num * SECTOR_SIZE, onceIO->sector,
                    onceIO->sec_num);
            tester->fail_num_++;
          } else {
            if (tester->debug_)
              fprintf(stderr, "write ok, start_num:%lu, sec_num:%u\n",
                      onceIO->sector, onceIO->sec_num);
          }
        }

        if (onceIO->data) {
          free(onceIO->data);
          onceIO->data = NULL;
        }
        delete onceIO;
      }
      finished_aio -= num_events;
      if (finished_aio <= 0 || num_events == 0) {
        break;
      }
    }

    if (tester->job_do_ != tester->job_num_) {
      run_test(tester);
    } else {
      tester->finish_ = true;
    }
  }
  return;
}

// 准备一次IO的写入数据
void prepareWriteIoData(OnceIO *onceIO, IoTest *tester) {
  assert(nullptr != onceIO);

  size_t io_size = onceIO->sec_num * SECTOR_SIZE;
  int ret = posix_memalign(&onceIO->data, PAGE_ALIGN_SIZE, io_size);
  assert(ret == 0);

  if (onceIO->optype == O_CMD_READ) {
    memset(onceIO->data, 0, io_size);
  }

  if (onceIO->optype == O_CMD_WRITE) {
    uint32_t offset = 0;
    // 同一扇区写入扇区号和指定魔术值之和
    for (uint64_t sector_no = onceIO->sector;
         sector_no < onceIO->sector + onceIO->sec_num; ++sector_no) {
      uint64_t info = sector_no + tester->magic_number_;
      for (uint32_t j = 0; j < SECTOR_SIZE / 8; ++j) {
        memcpy(reinterpret_cast<char *>(onceIO->data) + offset, &info,
               sizeof(info));
        offset += 8;
      }
    }
  }
}

void run_test(IoTest *tester) {
  int trigger_num = tester->parallel_ - tester->running_num_;
  static uint32_t io_num = 0;
  static size_t list_size = tester->io_list_.size();

  for (int i = 0; i < trigger_num && io_num < list_size; ++i) {
    OnceIO *onceIO = tester->io_list_[io_num];

    prepareWriteIoData(onceIO, tester);

    SubmitIoRequest(tester, onceIO);

    ++tester->running_num_;
    ++tester->job_do_;
    io_num++;
  }
}

void freePrepareIo(IoTest *tester) {
  for (auto io : tester->io_list_) {
    if (io->data) {
      free(io->data);
    }
    free(io);
  }
}

bool dumpIoList(IoTest *tester) {
  assert(!tester->io_list_file_.empty());

  FILE *fp = fopen(tester->io_list_file_.c_str(), "w");
  if (fp == NULL) return false;

  for (auto io : tester->io_list_) {
    fprintf(fp, "%d %lu %d\n", io->optype, io->sector, io->sec_num);
  }

  fclose(fp);
  return true;
}

bool prepareCheckIoByListOrder(IoTest *tester) {
  if (tester->io_list_file_.empty()) {
    fprintf(stderr, "io list file empty \n");
    help();
    return false;
  }

  size_t len = 1024;
  char *line = new char[len];
  ssize_t read;
  FILE *fp = fopen(tester->io_list_file_.c_str(), "r");
  if (fp == NULL) return false;

  freePrepareIo(tester);

  while ((read = getline(&line, &len, fp)) != -1) {
    int cmd_type;
    OnceIO *onceIO = new OnceIO();
    int ret =
        sscanf(line, "%d %lu %u", &cmd_type, &onceIO->sector, &onceIO->sec_num);
    assert(ret == 3);

    if (!cmd_type) {
      continue;
    } else {
      onceIO->optype = O_CMD_READ;
    }

    tester->io_list_.push_back(onceIO);
  }

  fclose(fp);
  return true;
}

void prepareCheckIoByListRandOrder(IoTest *tester) {
  prepareCheckIoByListOrder(tester);
  random_shuffle(tester->io_list_.begin(), tester->io_list_.end());
}

void prepareSequentialIo(IoTest *tester) {
  // 最近一次操作的起始扇区
  uint64_t last_start_sector = 0;
  // 最近一次操作的扇区数
  uint64_t last_sector_num = 0;

  for (int job_num = 0;
       last_start_sector + last_sector_num < tester->total_sector_num_ &&
       job_num < tester->job_num_;
       job_num++) {
    OnceIO *onceIO = new OnceIO();
    if (tester->write_ || tester->randwrite_) {
      onceIO->optype = O_CMD_WRITE;
    } else if (tester->check_ || tester->randcheck_) {
      onceIO->optype = O_CMD_READ;
    } else {
      onceIO->optype = (rand() % 2) ? O_CMD_READ : O_CMD_WRITE;
    }

    onceIO->sector = last_start_sector + last_sector_num;
    onceIO->sec_num = tester->op_sector_num_;

    if (onceIO->sector + onceIO->sec_num > tester->total_sector_num_) {
      onceIO->sec_num = tester->total_sector_num_ - onceIO->sector;
    }

    last_start_sector = onceIO->sector;
    last_sector_num = onceIO->sec_num;

    tester->io_list_.push_back(onceIO);
  }
}

void prepareRandomIo(IoTest *tester) {
  prepareSequentialIo(tester);
  random_shuffle(tester->io_list_.begin(), tester->io_list_.end());
}

void cron_job(evutil_socket_t fd, short what, void *ctx) {
  IoTest *tester = reinterpret_cast<IoTest *>(ctx);
  if (!tester->has_trigger_) {
    run_test(tester);
    tester->has_trigger_ = true;
  }

  printf("%s running: %d, total_job_num: %d, has_run_job: %d, fail_job: %lu\n",
         get_time().c_str(), tester->running_num_, tester->job_num_,
         tester->job_do_, tester->fail_num_);
  if (tester->finish_) {
    sleep(3);
    tester->free_event();
    printf("%s all finish \n", get_time().c_str());
  }
}

void help() {
  printf("c: category write/check/randwrite/randcheck/rw/randrw\n");
  printf("f: file \n");
  printf("p: parallel(if not set, the num will be dynamic adjusted)\n");
  printf("s: operation sector num \n");
  printf("i: io list file \n");
  printf("d: show debug info \n");
  printf("io_test -c randwrite -f /data/mysql/bfs/testfile -p 100 -s 1024 -i io_list.txt\n");
  printf("io_test -c randcheck -f /data/mysql/bfs/testfile -p 100 -s 1024 -i io_list.txt\n");
}

int main(int argc, char **argv) {
  IoTest *tester = new IoTest();
  int ch;
  while ((ch = getopt(argc, argv, "rdf:c:p:hw:s:i:g:e:j:k:")) != -1) {
    switch (ch) {
      case 'c':
        printf("option c:'%s'\n", optarg);
        tester->category_ = optarg;
        break;
      case 'f':
        printf("option f:'%s'\n", optarg);
        tester->file_ = optarg;
        break;
      case 'p':
        tester->parallel_ = atoi(optarg);
        assert(tester->parallel_ > 0);
        printf("parallel: %d\n", tester->parallel_);
        break;
      case 's':
        tester->op_sector_num_ = atoi(optarg);
        assert(tester->op_sector_num_ > 0);
        printf("operation sector num per time: %d\n", tester->op_sector_num_);
        break;
      case 'i':
        printf("option i:'%s'\n'", optarg);
        tester->io_list_file_ = optarg;
        break;
      case 'd':
        printf("show debug info\n");
        tester->debug_ = true;
        break;
      case 'h':
        help();
        return 0;
    }
  }

  if (tester->category_.empty()) {
    fprintf(stderr, "category empty, please use -c \n");
    return 0;
  }

  if (tester->file_.empty()) {
    fprintf(stderr, "file empty, please use -f \n");
    return 0;
  }

  int open_flags = O_RDWR | O_DSYNC | O_DIRECT;
  tester->fd_ = open64(tester->file_.c_str(), open_flags, 0644);
  assert(tester->fd_ > 0);

  int64_t file_size = lseek(tester->fd_, 0LL, SEEK_END);
  assert(file_size > 0);
  printf("file: %s\t file_size: %ld\n", tester->file_.c_str(), file_size);
  tester->total_sector_num_ = ((uint64_t)file_size) / SECTOR_SIZE;
  printf("total_sector_num: %ld\n", tester->total_sector_num_);

  tester->base_ = event_base_new();

  tester->timer_event_ =
      event_new(tester->base_, -1, EV_PERSIST, cron_job, tester);
  assert(tester->timer_event_);
  struct timeval timer_tv = {1, 0};
  event_add(tester->timer_event_, &timer_tv);

  tester->init_aio();
  srand(time(NULL));

  if (tester->category_ == "write") {
    printf("category: write\n");
    tester->write_ = true;
    prepareSequentialIo(tester);
    if (!tester->io_list_file_.empty()) {
      dumpIoList(tester);
    }
  } else if (tester->category_ == "check") {
    printf("category: check\n");
    tester->check_ = true;
    if (tester->io_list_file_.empty()) {
      prepareSequentialIo(tester);
    } else {
      prepareCheckIoByListOrder(tester);
    }
  } else if (tester->category_ == "randwrite") {
    printf("category: randwrite\n");
    tester->randwrite_ = true;
    prepareRandomIo(tester);
    if (!tester->io_list_file_.empty()) {
      dumpIoList(tester);
    }
  } else if (tester->category_ == "randcheck") {
    printf("category: randcheck \n");
    tester->randcheck_ = true;
    if (tester->io_list_file_.empty()) {
      prepareRandomIo(tester);
    } else {
      prepareCheckIoByListRandOrder(tester);
    }
  } else if (tester->category_ == "rw") {
    printf("category: rw \n");
    tester->rw_ = true;
    prepareSequentialIo(tester);
    if (!tester->io_list_file_.empty()) {
      dumpIoList(tester);
    }
  } else if (tester->category_ == "randrw") {
    printf("category: randrw\n");
    tester->randrw_ = true;
    prepareRandomIo(tester);
    if (!tester->io_list_file_.empty()) {
      dumpIoList(tester);
    }
  } else {
    fprintf(stderr, "error cmd:%s \n", tester->category_.c_str());
    return 0;
  }

  tester->job_num_ = tester->io_list_.size();
  printf("Actual total job num:%d\n", tester->job_num_);
  if (0 == tester->job_num_) {
    fprintf(stderr, "total job num is 0, exit!!\n");
    return 0;
  }

  printf("init_aio finish\n");
  int dret = event_base_dispatch(tester->base_);
  printf("to exit:%d \n", dret);
  event_base_free(tester->base_);

  return 0;
}
