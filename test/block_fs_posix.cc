#include <gtest/gtest.h>

#include "file_store_udisk.h"
#include "file_system.h"
#include "logging.h"

#define BLOCKFS_UNUSED_FD 10000

// 全局事件用于初始化环境
class BlockFsEnvironment : public testing::Environment {
private:
    const char *mount_point_ = "/mnt/mysql";
    const char *uuid_;
    bool master_ = true;

public:
    void SetUp() override {
        int ret = block_fs_mount("/data/blockfs/conf/bfs.cnf", master_);
        ASSERT_EQ(ret, 0);
        uuid_ = FileStore::Instance()->mount_config()->device_uuid_.c_str();
        // 设置测试文件中的日志等级
        Logger::set_min_level(LogLevel::FATAL);
    }
    void TearDown() override {
        int ret = block_fs_unmount(uuid_);
        EXPECT_EQ(ret, 0);
    }
};

TEST(TESTGROUP, test_mount_point_path) {
    char mount_point[64] = "/mnt/mysql/";
    LOG(FATAL) << "Test mount_point: " << FileStore::Instance()->super()->uxdb_mount_point().c_str();

    int32_t ret;
    struct stat st;

    // 0. 挂载点不为空
    ret = FileStore::Instance()->super()->uxdb_mount_point().size();
    EXPECT_GT(ret, 0);

    // 1. 根目录必须存在,根目录挂载文件系统后必须存在
    ret = block_fs_mkdir(mount_point, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EEXIST);
    ret = block_fs_mkdir(mount_point, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EEXIST);
    int32_t fd = block_fs_open(mount_point, O_RDWR);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EISDIR);

    ret = block_fs_stat(mount_point, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISDIR(st.st_mode), true);
    EXPECT_EQ(S_ISREG(st.st_mode), false);

    // 2. 根目录挂载文件系统不能被卸载
    // rmdir returns EBUSY if the directory to be removed
    // is the mount point for a mounted file system
    ret = block_fs_rmdir(mount_point);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EPERM | EBUSY);
}

TEST(TESTGROUP, verify_dir_path) {
    int32_t ret;

    // 创建文件目录不在挂载点目录下面认为是不同的文件系统
    char dirname1[64] = "/mnt/d5";  // 不合法的路径
    ret = block_fs_mkdir(dirname1, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);
    ret = block_fs_rmdir(dirname1);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);

    char dirname2[64] = "/mnt/datad2";  // 不合法的路径
    ret = block_fs_mkdir(dirname2, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);
    ret = block_fs_rmdir(dirname2);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);

    char dirname3[64] = "/mnt/datad2/f1";  // 不合法的路径
    ret = block_fs_mkdir(dirname3, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);

    char dirname4[64] = "/home/mnt/mysql/data/f1";  // 不合法的路径
    ret = block_fs_mkdir(dirname4, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EXDEV);

    char dirname5[64] = "/mnt/mysql/d1";
    ret = block_fs_mkdir(dirname5, S_IRWXU);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    // 文件夹如果已经存在则报错
    ret = block_fs_mkdir(dirname5, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EEXIST);

    char filename2[64] = "/mnt/mysql/d1/f2";
    ret = block_fs_create(filename2, 0644);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_rmdir(dirname5);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOTEMPTY | EEXIST);
    ret = block_fs_unlink(filename2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_rmdir(dirname5);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    // 删除不存在的文件夹
    ret = block_fs_rmdir(dirname5);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);
}

TEST(TESTGROUP, dir_test_with_file) {
    int32_t ret;

    // 前缀路径不存在返回 ENOENT
    char dirname6[64] = "/mnt/mysql/d2/d3";
    ret = block_fs_mkdir(dirname6, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    char filename1[64] = "/mnt/mysql/f1";
    ret = block_fs_create(filename1, 0644);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    // 文件已经存在, 创建和文件同名的文件夹
    // mkdir returns EEXIST if the named file exists
    ret = block_fs_mkdir(filename1, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EEXIST);
    // 删除和文件同名的文件夹
    ret = block_fs_rmdir(filename1);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOTDIR);

    // 如果要创建的文件夹前面的父文件夹不是目录
    char dirname7[64] = "/mnt/mysql/f1/d4";
    ret = block_fs_mkdir(dirname7, S_IRWXU);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOTDIR);

    ret = block_fs_unlink(filename1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    // 文件夹名字太长 ENAMETOOLONG

    // 文件前缀不存在写权限返回 EACCES

    // 如果两个文件夹相互软链接,再在文件夹上创建文件夹返回 ELOOP
    // mkdir returns ELOOP if too many symbolic links were
    // encountered in translating the pathname

    // 文件系统的文件夹的INODE不够了返回 ENOSPC
    // 测试多创建一些文件夹

    // mkdir returns EFAULT
    // if the path argument points outside the process's allocated address space
    // ret = block_fs_mkdir(nullptr, S_IRWXU);
    // EXPECT_EQ(ret, -1);
    // EXPECT_EQ(errno, EFAULT);
}

TEST(TESTGROUP, test_file) {
    int32_t ret;

    // 不是文件系统挂载的目录
    char filename1[64] = "/mnt/data.imdb";
    int32_t fd = block_fs_open(filename1, O_WRONLY | O_CREAT);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EXDEV);

    char filename2[64] = "/mnt/dataf2.imdb";
    fd = block_fs_open(filename2, O_WRONLY | O_CREAT);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EXDEV);

    char filename3[64] = "/mnt/f2.imdb";
    fd = block_fs_open(filename3, O_WRONLY | O_CREAT);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EXDEV);

    char filename4[64] = "/home/mnt/mysql/f2.imdb";
    fd = block_fs_open(filename4, O_WRONLY | O_CREAT);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EXDEV);

    char filename[64] = "/mnt/mysql/f1.imdb";
    // 1. 打开不存在的文件, 并且未使用O_CREAT应该失败
    int32_t fd1 = block_fs_open(filename, O_WRONLY);
    EXPECT_EQ(fd1, -1);
    EXPECT_EQ(errno, ENOENT);
    // 2. 打开不存在的文件, 并且使用O_CREAT应该成功
    int32_t fd2 = block_fs_open(filename, O_CREAT);
    EXPECT_GT(fd2, -1);
    EXPECT_EQ(errno, 0);

    // 3. 打开存在的文件, 并且使用O_CREAT和O_EXCL标志应该失败
    // fd = block_fs_open(filename, O_CREAT, 0 | O_EXCL);
    // EXPECT_EQ(ret, -1);
    // EXPECT_EQ(errno, EEXIST);

    // 4. 打开存在的文件, 并且使用O_CREAT应该成功
    fd1 = block_fs_open(filename, O_CREAT, 0);
    EXPECT_GT(fd2, -1);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(fd1, fd2) << "Open of existing file did not return right fd, fd: "
                        << fd1;

    ret = block_fs_close(fd1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    // 如果文件的 文件夹前缀不是文件夹ENOTDIR

    // 文件名字很长 ENAMETOOLONG

    // desc="open returns EACCES when the required permissions (for reading
    // and/or writing) are denied for the given flags" expect EACCES -u 65534 -g
    // 65534 open ${n1} O_RDONLY,O_TRUNC

    // desc="open returns EACCES when O_CREAT is specified,
    // the file does not exist, and the directory in which
    // it is to be created does not permit writing"
    // expect EACCES -u 65534 -g 65534 open ${n1} O_RDONLY,O_CREAT 0644

    // desc="open returns ELOOP
    // if too many symbolic links were encountered in translating the pathname"

    // desc="open returns EROFS
    // if the named file resides on a read-only file system, and the file is to
    // be modified" expect EROFS open ${n0}/${n1} O_WRONLY expect EROFS open
    // ${n0}/${n1} O_RDWR expect EROFS open ${n0}/${n1} O_RDONLY,O_TRUNC

    // https://www.cnblogs.com/kilo-qian/p/9664545.html
    // expect 0 create ${n0} 0644
    // expect 0 open ${n0} O_RDONLY,O_SHLOCK : open ${n0}
    // O_RDONLY,O_SHLOCK,O_NONBLOCK expect "EWOULDBLOCK|EAGAIN" open ${n0}
    // O_RDONLY,O_EXLOCK : open ${n0} O_RDONLY,O_EXLOCK,O_NONBLOCK expect
    // "EWOULDBLOCK|EAGAIN" open ${n0} O_RDONLY,O_SHLOCK : open ${n0}
    // O_RDONLY,O_EXLOCK,O_NONBLOCK expect "EWOULDBLOCK|EAGAIN" open ${n0}
    // O_RDONLY,O_EXLOCK : open ${n0} O_RDONLY,O_SHLOCK,O_NONBLOCK

    // desc="open returns ENOSPC when O_CREAT is specified,
    // the file does not exist, and there are no free inodes
    // on the file system on which the file is being created"

    // desc="open returns ETXTBSY when the file is a pure procedure (shared
    // text) file that is being executed and the open() system call requests
    // write access" cp -pf `which sleep` ${n0}
    // ./${n0} 3 &
    // expect ETXTBSY open ${n0} O_WRONLY
    // expect ETXTBSY open ${n0} O_RDWR
    // expect ETXTBSY open ${n0} O_RDONLY,O_TRUNC

    // desc="open returns EFAULT
    // if the path argument points outside the process's allocated address
    // space"

    // desc="open returns EEXIST when O_CREAT and O_EXCL were specified and the
    // file exists" create_file ${type} ${n0} expect EEXIST open ${n0}
    // O_CREAT,O_EXCL 0644

    // desc="open may return EINVAL when an attempt was made to open a
    // descriptor with an illegal combination of O_RDONLY, O_WRONLY, and O_RDWR"
    // expect 0 create ${n0} 0644 expect "0|EINVAL" open ${n0} O_RDONLY,O_RDWR
    // expect "0|EINVAL" open ${n0} O_WRONLY,O_RDWR expect "0|EINVAL" open ${n0}
    // O_RDONLY,O_WRONLY,O_RDWR
}

TEST(TESTGROUP, test_file_close) {
    char filename[64] = "/mnt/mysql/file.txt";
    int32_t fd;
    int32_t ret;

    ret = block_fs_close(BLOCKFS_UNUSED_FD);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EBADF);

    fd = block_fs_open(filename, O_CREAT, 0);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST(TESTGROUP, test_file_unlink) {
    char filename[64] = "/mnt/mysql/file.txt";
    char dirname[64] = "/mnt/mysql/somewhere";
    int fd;
    int ret;

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    fd = block_fs_open(filename, O_CREAT, 0);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_mkdir(dirname, S_IRWXU);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(dirname);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EISDIR);

    ret = block_fs_rmdir(dirname);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    // desc="unlink returns ENOTDIR if a component of the path prefix is not a
    // directory" desc="unlink returns ENAMETOOLONG if a component of a pathname
    // exceeded {NAME_MAX} characters" desc="unlink returns ENAMETOOLONG if an
    // entire path name exceeded {PATH_MAX} characters" desc="unlink returns
    // ENOENT if the named file does not exist" desc="unlink returns EACCES when
    // search permission is denied for a component of the path prefix"
    // desc="unlink returns EACCES when write permission is denied on the
    // directory containing the link to be removed"

    // desc="unlink returns ELOOP if too many symbolic links were encountered in
    // translating the pathname"

    // desc="unlink may return EPERM if the named file is a directory"
    // expect 0 mkdir $ { n0 }
    // 0755
    // todo Linux "According to POSIX: EPERM - The file named by path is a
    // directory, and either the calling process does not have appropriate
    // privileges, or the implementation prohibits using unlink() on
    // directories." expect "0|EPERM" unlink ${n0} expect "0|ENOENT" rmdir ${n0}

    // desc="unlink returns EROFS if the named file resides on a read-only file
    // system"

    // desc="unlink returns EFAULT if the path argument points outside the
    // process's allocated address space"

    // desc="An open file will not be immediately freed by unlink"
}

TEST(TESTGROUP, test_access) {
    char filename[64] = "/mnt/mysql/filename";
    int32_t fd;
    int32_t ret;

    ret = block_fs_access(filename, R_OK);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_access(filename, W_OK);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_access(filename, X_OK);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_access(filename, F_OK);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    fd = block_fs_open(filename, O_WRONLY | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_access(filename, R_OK);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_access(filename, W_OK);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_access(filename, X_OK);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_access(filename, F_OK);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

class FileTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
        ::free(aligned_buffer);
        aligned_buffer = nullptr;
    }

    char filename[64] = "/mnt/mysql/filename";
    char dirname[64] = "/mnt/mysql/somewhere";
    char *aligned_buffer = nullptr;
};

TEST_F(FileTest, file_test) {
    char buf[8192] = {0};
    uint64_t count;
    uint64_t offset;
    int32_t fd;
    int32_t ret;
    struct stat st;

    // 检查一个不存在的文件
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_EQ(S_ISREG(st.st_mode), false);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);

    fd = block_fs_open(filename, O_WRONLY | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, 0);

    // Buffer地址,写的count,Offset都需要对齐
    // O_DIRECT模式三者缺一不可
    void *raw_buffer = nullptr;
    ret = ::posix_memalign(&raw_buffer, 512, 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(raw_buffer, nullptr);
    aligned_buffer = static_cast<char *>(raw_buffer);

    // 写入字节count不是512内存对齐的情况
    count = 8 * K - 1;
    offset = 0;
    ret = block_fs_pwrite(fd, aligned_buffer, count, offset);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    // 写入字节offset不是512内存对齐的情况
    count = 8 * K;
    offset = 1;
    ret = block_fs_pwrite(fd, aligned_buffer, count, offset);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    // 三个条件都没有对齐的情况
    count = 8 * K - 1;
    offset = 1;
    ret = block_fs_pwrite(fd, buf, count, offset);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    // 上面几个测试不会对文件造成影响
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, 0);

    // 文件写放大
    count = 8 * K;
    offset = 0;
    ret = block_fs_pwrite(fd, aligned_buffer, count, offset);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, count);

    // 原地truncate不会改变大小
    ret = block_fs_ftruncate(fd, st.st_size);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, count);

    // 扩容文件到32K
    ret = block_fs_ftruncate(fd, 32 * K);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, 32 * K);

    // 扩容文件到16M大小
    ret = block_fs_ftruncate(fd, kBlockFsBlockSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize);

    // 扩容文件到16M + 8K 大小
    ret = block_fs_ftruncate(fd, kBlockFsBlockSize + 8 * K);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize + 8 * K);

    // 扩容文件到2倍16M大小
    ret = block_fs_ftruncate(fd, kBlockFsBlockSize * 2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize * 2);

    // 扩容文件到2倍16M + 8K大小
    ret = block_fs_ftruncate(fd, kBlockFsBlockSize * 2 + 8 * K);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize * 2 + 8 * K);

    // 扩容文件到一个FileBlock最大大小
    ret = block_fs_ftruncate(fd, kBlockFsBlockSize * kBlockFsFileBlockCapacity);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize * kBlockFsFileBlockCapacity);

    // 扩容文件到一个FileBlock最大大小 + 8192
    ret = block_fs_ftruncate(
        fd, kBlockFsBlockSize * kBlockFsFileBlockCapacity + 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_stat(filename, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), true);
    EXPECT_EQ(S_ISDIR(st.st_mode), false);
    EXPECT_EQ(st.st_size, kBlockFsBlockSize * kBlockFsFileBlockCapacity + 8192);

    ret = block_fs_mkdir(dirname, S_IRWXU);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_stat(dirname, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(S_ISREG(st.st_mode), false);
    EXPECT_EQ(S_ISDIR(st.st_mode), true);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_rmdir(dirname);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(FileTest, posix_write) {
    int fd;
    int ret;
    uint64_t count = 8 * K;

    void *raw_buffer = nullptr;
    ret = ::posix_memalign(&raw_buffer, 512, 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(raw_buffer, nullptr);
    aligned_buffer = static_cast<char *>(raw_buffer);
    ::memset(aligned_buffer, '1', 128);
    ::memset(aligned_buffer + 128, '2', 128);
    ::memset(aligned_buffer + 256, '3', 128);
    ::memset(aligned_buffer + 384, '4', 128);

    ret = block_fs_write(BLOCKFS_UNUSED_FD, aligned_buffer, count);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    fd = block_fs_open(filename, O_WRONLY | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_write(fd, aligned_buffer, count);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);

    ret = block_fs_lseek(fd, 0, SEEK_SET);
    EXPECT_EQ(ret, 0);

    char tmp_buffer[512];
    ret = block_fs_read(fd, tmp_buffer, sizeof(tmp_buffer));
    EXPECT_EQ(ret, sizeof(tmp_buffer));
    for (int i = 0; i < 128; ++i) {
        EXPECT_EQ(tmp_buffer[i], '1');
    }
    for (int i = 128; i < 256; ++i) {
        EXPECT_EQ(tmp_buffer[i], '2');
    }
    for (int i = 256; i < 384; ++i) {
        EXPECT_EQ(tmp_buffer[i], '3');
    }
    for (int i = 384; i < 512; ++i) {
        EXPECT_EQ(tmp_buffer[i], '4');
    }

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_mkdir(dirname, S_IRWXU);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    // open目前只支持打开文件
    fd = block_fs_open(dirname, O_DIRECTORY);
    EXPECT_EQ(fd, -1);
    EXPECT_EQ(errno, EISDIR);

    ret = block_fs_rmdir(dirname);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(FileTest, posix_pwrite) {
    int fd;
    int ret;
    uint64_t count = 8192;

    void *raw_buffer = nullptr;
    ret = ::posix_memalign(&raw_buffer, 512, 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(raw_buffer, nullptr);
    aligned_buffer = static_cast<char *>(raw_buffer);

    ret = block_fs_pread(BLOCKFS_UNUSED_FD, aligned_buffer, count, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    fd = block_fs_open(filename, O_WRONLY | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_pread(fd, aligned_buffer, count, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_pwrite(fd, aligned_buffer, count, 0);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);

    ret = block_fs_pread(fd, aligned_buffer, count, 0);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

class RenameTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    void SetUp() override {
    }
    void TearDown() override {
        if (aligned_buffer) {
            ::free(aligned_buffer);
            aligned_buffer = nullptr;
        }
    }

    int32_t fd;
    int32_t fd2;
    int32_t ret;
    char *aligned_buffer = nullptr;
};

TEST_F(RenameTest, exception_test) {
    char filename1[64] = "/mnt/mysql/file1";
    char filename2[64] = "/mnt/mysql/unknown/file2";

    // filename1不存在
    ret = block_fs_rename(filename1, filename2);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_create(filename1, 0644);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    // filename2父文件夹不存在
    ret = block_fs_rename(filename1, filename2);
    EXPECT_EQ(ret, -1);

    ret = block_fs_unlink(filename1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(RenameTest, normal_test) {
    char filename1[64] = "/mnt/mysql/1.log";
    char filename2[64] = "/mnt/mysql/2.log";
    char filename3[64] = "/mnt/mysql/3.log";

    fd = block_fs_open(filename1, O_RDWR | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_rename(filename1, filename2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    void *raw_buffer = nullptr;
    ret = ::posix_memalign(&raw_buffer, 512, 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(raw_buffer, nullptr);
    aligned_buffer = static_cast<char *>(raw_buffer);

    uint64_t count = 8 * K;
    ret = block_fs_write(fd, aligned_buffer, count);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);

    ret = block_fs_rename(filename2, filename3);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename3);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(RenameTest, dest_file_exist) {
    char filename1[64] = "/mnt/mysql/1.log";
    char filename2[64] = "/mnt/mysql/2.log";

    fd = block_fs_open(filename1, O_RDWR | O_CREAT);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    void *raw_buffer = nullptr;
    ret = ::posix_memalign(&raw_buffer, 512, 8192);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_NE(raw_buffer, nullptr);
    aligned_buffer = static_cast<char *>(raw_buffer);

    uint64_t count = 8 * K;
    ret = block_fs_write(fd, aligned_buffer, count);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);

    fd2 = block_fs_open(filename2, O_RDWR | O_CREAT);
    EXPECT_GT(fd2, -1);
    EXPECT_EQ(errno, 0);
    ret = block_fs_write(fd2, aligned_buffer, count);
    EXPECT_EQ(ret, count);
    EXPECT_EQ(errno, 0);
    ret = block_fs_close(fd2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_rename(filename1, filename2);
    EXPECT_EQ(ret, -1);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_unlink(filename1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    ret = block_fs_unlink(filename2);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

class BlockFsFtruncateTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    void SetUp() override {
        ret = block_fs_create(name0, 0644);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(errno, 0);
    }
    void TearDown() override {
        ret = block_fs_unlink(name0);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(errno, 0);
    }

    char const *name0 = "/mnt/mysql/name0";
    char const *name1 = "/mnt/mysql/name0/name1";
    char const *dir0 = "/mnt/mysql/dir0";
    char const *name2 = "/mnt/mysql/dir0/name0";
    int32_t fd;
    int32_t ret;
};

TEST_F(BlockFsFtruncateTest, ftruncate_test) {
    fd = block_fs_open(name0, O_RDWR);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_ftruncate(fd, 1234567);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    struct stat st;
    ret = block_fs_fstat(fd, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(st.st_size, 1234567);
    ret = block_fs_stat(name0, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(st.st_size, 1234567);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    fd = block_fs_open(name0, O_WRONLY);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_ftruncate(fd, 567);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_fstat(fd, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(st.st_size, 567);
    ret = block_fs_stat(name0, &st);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
    EXPECT_EQ(st.st_size, 567);

    ret = block_fs_ftruncate(fd, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(BlockFsFtruncateTest, ftruncate_readonly_file) {
    fd = block_fs_open(name0, O_RDONLY);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    // TODO: 应该报错的
    ret = block_fs_ftruncate(fd, 1);
    //    EXPECT_EQ(ret, -1);
    //    EXPECT_EQ(errno, EINVAL);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_close(fd);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(BlockFsFtruncateTest, length_is_negative) {
    // ftruncate returns EINVAL if the length argument was less than 0
    fd = block_fs_open(name0, O_RDWR);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_ftruncate(fd, -1);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    fd = block_fs_close(fd);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    fd = block_fs_open(name0, O_WRONLY);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);

    ret = block_fs_ftruncate(fd, -99999);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    fd = block_fs_close(fd);
    EXPECT_GT(fd, -1);
    EXPECT_EQ(errno, 0);
}

class TruncateTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    void SetUp() override {
    }
    void TearDown() override {
    }

    char const *name0 = "/mnt/mysql/name0";
    char const *name1 = "/mnt/mysql/name0/name1";
    char const *dir0 = "/mnt/mysql/dir0";
    char const *name2 = "/mnt/mysql/dir0/name0";
    int32_t fd;
    int32_t ret;
};

TEST_F(TruncateTest, truncate_test) {
    // truncate returns ENOTDIR if a component of the path prefix is not a
    // directory
    ret = block_fs_truncate(name1, 123);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    // truncate returns ENOENT if the named file does not exist
    ret = block_fs_truncate(name2, 123);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, ENOENT);

    // truncate returns EISDIR if the named file is a directory
    ret = block_fs_mkdir(dir0, 0755);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_truncate(dir0, 123);
    EXPECT_EQ(ret, -1);
    // EXPECT_EQ(errno, EISDIR);
    EXPECT_EQ(errno, ENOENT);

    ret = block_fs_rmdir(dir0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(TruncateTest, length_is_negative) {
    // ftruncate returns EINVAL if the length argument was less than 0
    ret = block_fs_create(name0, 0644);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);

    ret = block_fs_truncate(name0, -1);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    ret = block_fs_truncate(name0, -99999);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINVAL);

    ret = block_fs_unlink(name0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(errno, 0);
}

TEST_F(TruncateTest, input_is_nullptr) {
    // truncate returns EFAULT
    // if the path argument points outside the process's allocated address space
    ret = block_fs_truncate(nullptr, 123);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EFAULT);
}

int main(int argc, char *argv[]) {
    // https://www.cnblogs.com/coderzh/archive/2009/04/10/1432789.html
    // https://blog.csdn.net/FlushHip/article/details/87936876
    // testing::GTEST_FLAG(output) = "xml:";
    // testing::GTEST_FLAG(color) = "yes";
    // testing::GTEST_FLAG(print_time) = true;
    // testing::GTEST_FLAG(repeat) = 100;
    testing::AddGlobalTestEnvironment(new BlockFsEnvironment());
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}