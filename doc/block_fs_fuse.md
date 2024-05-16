## BlockFS支持FUSE标准本地文件系统挂载

### 2. FUSE介绍

FUSE 是Linux Kernel的特性之一：

一个用户态文件系统框架，a userspace filesystem framework

形象的说就是可以在用户态运行一个程序，程序暴露出一个FUSE文件系统，

对这个文件系统进行的读写操作都会被转给用户态的程序处理。

FUSE由内核模块 fuse.ko 和用户空间的动态链接库 libfuse.* 组成，

### https://man7.org/linux/man-pages/man8/mount.fuse3.8.html

如果要开发使用fuse的用户态程序，需要安装 fuse-devel

```sh
ubuntu:

apt-get install fuse

centos:

yum install fuse-devel
```


#### Fuse control filesystem


加载fuse.ko后，可以用下面的命令加载fusectl fs：

```sh
mount -t fusectl none /sys/fs/fuse/connections

```

每个使用fuse的进程有一个对应的目录：

```sh
$ ls  /sys/fs/fuse/connections
38  42
```

fuse提供了 fuse 和 fuseblk 两种文件系统类型，

可以作为mount命令的 -t 参数的参数值, 文档给出的挂载选项：

```sh
'fd=N'

  The file descriptor to use for communication between the userspace
  filesystem and the kernel.  The file descriptor must have been
  obtained by opening the FUSE device ('/dev/fuse').

'rootmode=M'

  The file mode of the filesystem's root in octal representation.

'user_id=N'

  The numeric user id of the mount owner.

'group_id=N'

  The numeric group id of the mount owner.

'default_permissions'

  By default FUSE doesn't check file access permissions, the
  filesystem is free to implement its access policy or leave it to
  the underlying file access mechanism (e.g. in case of network
  filesystems).  This option enables permission checking, restricting
  access based on file mode.  It is usually useful together with the
  'allow_other' mount option.

'allow_other'

  This option overrides the security measure restricting file access
  to the user mounting the filesystem.  This option is by default only
  allowed to root, but this restriction can be removed with a
  (userspace) configuration option.

'max_read=N'

  With this option the maximum size of read operations can be set.
  The default is infinite.  Note that the size of read requests is
  limited anyway to 32 pages (which is 128kbyte on i386).

'blksize=N'

  Set the block size for the filesystem.  The default is 512.  This
  option is only valid for 'fuseblk' type mounts.

```


### 3. FUSE用户态程序

BFS中fuse_main的输入参数: argc、argv，命令行参数

挂载参数，有参数值，BFS中设置的参数是:

```sh
allow_other,direct_io,entry_timeout=0.5,attr_timeout=0.5,auto_umount
```


BFS中fuse_main的输入参数：op，文件操作函数

fuse_operations 在文件 /usr/include/fuse3/fuse.h 中定义

FUSE版本和支持的命令行参数

```sh
luotang@10-23-227-66:~/blockfs/build$ sudo ./app/bfs_mount -V
[876191 20210120 11:06:15.627125Z][INFO][app/bfs_mount.cc:329] FUSE version: 3.10.1
FUSE library version 3.10.1
using FUSE kernel interface version 7.31
fusermount3 version: 3.10.1
[876191 20210120 11:06:15.627930Z][ERROR][app/bfs_mount.cc:362] fuse mount exit

luotang@10-23-227-66:~/blockfs/build$ sudo ./app/bfs_mount -h
[sudo] password for luotang:
[875750 20210120 11:04:46.009927Z][INFO][app/bfs_mount.cc:329] FUSE version: 3.10.1
usage: ./app/bfs_mount [options] <mountpoint>

FUSE options:
    -h   --help            print help
    -V   --version         print version
    -d   -o debug          enable debug output (implies -f)
    -f                     foreground operation
    -s                     disable multi-threaded operation
    -o clone_fd            use separate fuse device fd for each thread
                           (may improve performance)
    -o max_idle_threads    the maximum number of idle worker threads
                           allowed (default: 10)
    -o kernel_cache        cache files in kernel
    -o [no]auto_cache      enable caching based on modification times (off)
    -o umask=M             set file permissions (octal)
    -o uid=N               set file owner
    -o gid=N               set file group
    -o entry_timeout=T     cache timeout for names (1.0s)
    -o negative_timeout=T  cache timeout for deleted names (0.0s)
    -o attr_timeout=T      cache timeout for attributes (1.0s)
    -o ac_attr_timeout=T   auto cache timeout for attributes (attr_timeout)
    -o noforget            never forget cached inodes
    -o remember=T          remember cached inodes for T seconds (0s)
    -o modules=M1[:M2...]  names of modules to push onto filesystem stack
    -o allow_other         allow access by all users
    -o allow_root          allow access by root
    -o auto_unmount        auto unmount on process termination

Options for subdir module:
    -o subdir=DIR           prepend this directory to all paths (mandatory)
    -o [no]rellinks         transform absolute symlinks to relative

Options for iconv module:
    -o from_code=CHARSET   original encoding of file names (default: UTF-8)
    -o to_code=CHARSET     new encoding of the file names (default: UTF-8)
[875750 20210120 11:04:46.010178Z][ERROR][app/bfs_mount.cc:362] fuse mount exit
luotang@10-23-227-66:~/blockfs/build$
```


##### 实际运行效果
```sh
luotang@10-23-227-66:~/blockfs/build$ sudo ./app/bfs_mount /home/luotang/bfs/
[877654 20210120 11:11:18.315592Z][INFO][app/bfs_mount.cc:329] FUSE version: 3.10.1
Protocol version: 7.27
Capabilities:
        FUSE_CAP_WRITEBACK_CACHE
        FUSE_CAP_ASYNC_READ
        FUSE_CAP_POSIX_LOCKS
        FUSE_CAP_ATOMIC_O_TRUNC
        FUSE_CAP_EXPORT_SUPPORT

........

[877656 20210120 11:11:20.468840Z][INFO][lib/file_handle.cc:80] read file meta success, free num:99979
[877656 20210120 11:11:20.468858Z][INFO][lib/file_block_handle.cc:14] total file block num: 108388
[877656 20210120 11:11:20.544594Z][INFO][lib/file_block_handle.cc:102] read file block meta success, free num:108367
[877656 20210120 11:11:20.547188Z][WARN][lib/journal_handle.cc:61] load journal success, journal head_: -1 journal tail_: -1 min_seq_no_: 0 max_seq_no_: 0 available_seq_no_: 1
[877656 20210120 11:11:20.547206Z][ERROR][lib/journal_handle.cc:104] I am not master, cannot replay journal
[877656 20210120 11:11:20.547215Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[877656 20210120 11:11:20.547219Z][INFO][lib/super_block.cc:173] mount point has been configured
[877656 20210120 11:11:20.547221Z][INFO][lib/file_store_udisk.cc:1026] create fs mount point: /mnt/mysql/data/

........

[877657 20210120 11:12:59.788906Z][INFO][lib/file_store_udisk.cc:228] open directory: /mnt/mysql/data/
[877657 20210120 11:12:59.788975Z][INFO][lib/dir_handle.cc:615] open directory /mnt/mysql/data/
[877657 20210120 11:12:59.789004Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[877657 20210120 11:12:59.789011Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[877656 20210120 11:12:59.789075Z][INFO][app/bfs_mount.cc:149] readdir: /mnt/mysql/data/
[877656 20210120 11:12:59.789100Z][INFO][lib/directory.cc:103] scan directory: /mnt/mysql/data/#innodb_temp/
[877656 20210120 11:12:59.789108Z][INFO][lib/directory.cc:103] scan directory: /mnt/mysql/data/luotang/
[877656 20210120 11:12:59.789122Z][INFO][lib/directory.cc:103] scan directory: /mnt/mysql/data/sys/
[877656 20210120 11:12:59.789127Z][INFO][lib/directory.cc:121] scan file: undo_002
[877656 20210120 11:12:59.789132Z][INFO][lib/directory.cc:121] scan file: ib_logfile0
[877656 20210120 11:12:59.789137Z][INFO][lib/directory.cc:121] scan file: ibtmp1
[877656 20210120 11:12:59.789148Z][INFO][lib/directory.cc:121] scan file: ibdata1
[877656 20210120 11:12:59.789153Z][INFO][lib/directory.cc:121] scan file: ib_logfile1
[877656 20210120 11:12:59.789157Z][INFO][lib/directory.cc:121] scan file: mysql.ibd
[877656 20210120 11:12:59.789162Z][INFO][lib/directory.cc:121] scan file: #ib_16384_0.dblwr
[877656 20210120 11:12:59.789166Z][INFO][lib/directory.cc:121] scan file: #ib_16384_1.dblwr
[877656 20210120 11:12:59.789170Z][INFO][lib/directory.cc:121] scan file: undo_001
[877656 20210120 11:12:59.789180Z][INFO][lib/directory.cc:49] 1611112379 2021-01-20 11:12:59
[877656 20210120 11:12:59.789284Z][INFO][lib/file_store_udisk.cc:318] stat path: /mnt/mysql/data/#innodb_temp
[877656 20210120 11:12:59.789294Z][INFO][lib/file_store_udisk.cc:327] stat path: /mnt/mysql/data/#innodb_temp
[877656 20210120 11:12:59.789311Z][INFO][lib/file_handle.cc:694] get created file name: /mnt/mysql/data/#innodb_temp
[877656 20210120 11:12:59.789319Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/#innodb_tem

........



查看挂载信息:
fusectl /sys/fs/fuse/connections fusectl rw,relatime 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,relatime 0 0
tracefs /sys/kernel/debug/tracing tracefs rw,relatime 0 0
tmpfs /run/user/1002 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1002,gid=1002 0 0
tmpfs /run/user/1001 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1001,gid=1001 0 0
tmpfs /run/user/1000 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1000,gid=1000 0 0
bfs_mount /home/luotang/bfs fuse.bfs_mount rw,nosuid,nodev,relatime,user_id=0,group_id=0,allow_other 0 0
luotang@10-23-227-66:~$ cat /proc/mounts


luotang@10-23-227-66:~$ sudo umount /home/luotang/bfs
[sudo] password for luotang:
luotang@10-23-227-66:~$


[877657 20210120 11:12:59.789965Z][INFO][lib/file_handle.cc:135] transformPath dirname: /mnt/mysql/data/
[877657 20210120 11:12:59.789967Z][INFO][lib/file_handle.cc:136] transformPath filename: undo_001
[877657 20210120 11:12:59.790015Z][INFO][lib/dir_handle.cc:657] close directory name: /mnt/mysql/data/ fd: 0

[877654 20210120 11:15:03.637325Z][ERROR][app/bfs_mount.cc:362] fuse mount exit
luotang@10-23-227-66:~/blockfs/build$

```



### 4. 参考文档

* [Linux FUSE(用户态文件系统)的使用](https://www.lijiaocn.com/%E6%8A%80%E5%B7%A7/2019/01/21/linux-fuse-filesystem-in-userspace-usage.html)
* [Linux下使用fuse编写自己的文件系统](https://blog.csdn.net/stayneckwind2/article/details/82876330)
* [fuse文件系统分析(一)](https://blog.csdn.net/wyt357359/article/details/85255366?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-7.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-7.control)
* [用户态文件系统fuse学习](https://blog.csdn.net/ty_laurel/article/details/51685193?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-2.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromBaidu-2.control)
* [FUSE协议解析](https://blog.csdn.net/weixin_34372728/article/details/91949774?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-3&spm=1001.2101.3001.4242)
* [吴锦华/明鑫: 用户态文件系统(FUSE)框架分析和实战](https://blog.csdn.net/juS3Ve/article/details/78237236?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-11.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-11.control)
* [Fuse文件系统优化方案](https://blog.csdn.net/lzpdz/article/details/50222335?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-6&spm=1001.2101.3001.4242)
