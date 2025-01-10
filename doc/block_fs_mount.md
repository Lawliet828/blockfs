## block_fs_mount工具介绍

#### 1. bfs工具参数
```sh
$ sudo ./build/tool/block_fs_mount -v
[46652 20210202 14:13:25.986591Z][INFO][tool/block_fs_mount.cc:9] Build time    : Feb  2 2021 11:58:46
[46652 20210202 14:13:25.986648Z][INFO][lib/file_system.cc:23] bfs version: 21.02.02-dc860f7
[46652 20210202 14:13:25.986646Z][INFO][tool/block_fs_mount.cc:10] Build version : 21.02.02-dc860f7
[46652 20210202 14:13:25.986653Z][INFO][tool/block_fs_mount.cc:11] Run options:
[46652 20210202 14:13:25.986655Z][INFO][tool/block_fs_mount.cc:12]  -c, --config   /data/blockfs/conf/bfs.cnf
[46652 20210202 14:13:25.986659Z][INFO][tool/block_fs_mount.cc:14]  -v, --version  Print the version.
[46652 20210202 14:13:25.986661Z][INFO][tool/block_fs_mount.cc:15]  -h, --help     Print help info.
```

##### 参数解释：

-c 指定配置文件，否则默认是/data/blockfs/conf/bfs.cnf，配置文件的模版看下面

#### 2. bfs配置文件

[bfs.cnf](../conf/bfs.cnf)

##### 配置解释

common配置项：

log_level 必须配置，配置block_fs的代码debug打印等级



bfs配置项：

device_uuid 必须配置，需要使用block_fs_tool查看uuid得到uuid，相关操作查看tool的使用

fuse配置项：

fuse_attribute_timeout 选配，默认是5.0s

fuse_entry_timeout 选配，默认是5.0s


###  3.  bfs运行

```
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_mount
[58499 20210202 14:51:29.774293Z][INFO][lib/file_system.cc:23] bfs version: 21.02.02-dc860f7
[58499 20210202 14:51:29.774357Z][INFO][lib/file_store_udisk.cc:70] run as master node: true
[58499 20210202 14:51:29.774453Z][INFO][lib/config_load.cc:30] log level: INFO
[58499 20210202 14:51:29.774462Z][INFO][lib/config_load.cc:49] fuse enable: true
[58499 20210202 14:51:29.774471Z][INFO][lib/config_load.cc:63] fuse mount point: /home/luotang/bfs/
[58499 20210202 14:51:29.774484Z][INFO][lib/config_load.cc:90] fuse new_fuse_thread: false
[58499 20210202 14:51:29.774525Z][INFO][lib/block_device.cc:413] device: /dev/. not allowed, skip it
[58499 20210202 14:51:29.774532Z][INFO][lib/block_device.cc:413] device: /dev/.. not allowed, skip it
[58499 20210202 14:51:29.774539Z][WARN][lib/block_device.cc:379] master node need to set nomerges flag
[58499 20210202 14:51:29.774578Z][INFO][lib/block_device.cc:398] set /dev/vdf nomerges 2 successfully
[58499 20210202 14:51:29.774594Z][INFO][lib/block_device.cc:447] open block device /dev/vdf success
[58499 20210202 14:51:29.774600Z][INFO][lib/block_device.cc:214] get block device size: 536870912000 B
[58499 20210202 14:51:29.774603Z][INFO][lib/block_device.cc:234] get block device blk size: 512
[58499 20210202 14:51:29.774746Z][INFO][lib/shm_manager.cc:59] read super block success
[58499 20210202 14:51:29.774753Z][INFO][lib/shm_manager.cc:60] mount point: /mnt/mysql/data/
[58499 20210202 14:51:29.774755Z][INFO][lib/shm_manager.cc:61] block_data_start_offset: 603979776
[58499 20210202 14:51:29.774758Z][INFO][lib/shm_manager.cc:66] shm name: blockfs_shm_6a4d31e7-9fc7-427d-8054-c8fa7028f8d4
[58499 20210202 14:51:29.774764Z][INFO][lib/shm_manager.cc:67] shm size: 603979776
[58499 20210202 14:51:29.774768Z][INFO][lib/shm_manager.cc:72] let's look mem usage:
[58499 20210202 14:51:29.774773Z][INFO][lib/shm_manager.cc:73] totalram: 67318837248
[58499 20210202 14:51:29.774775Z][INFO][lib/shm_manager.cc:74] freeram: 5929328640
[58499 20210202 14:51:29.774781Z][INFO][lib/shm_manager.cc:75] sharedram: 10539008
[58499 20210202 14:51:29.774784Z][INFO][lib/shm_manager.cc:76] need shm: 603979776
[58499 20210202 14:51:29.774791Z][INFO][lib/shm_manager.cc:205] using posix mem aligned meta
[58499 20210202 14:51:31.836615Z][INFO][lib/shm_manager.cc:228] read all meta into shm success
[58499 20210202 14:51:31.836663Z][INFO][lib/shm_manager.cc:263] init and register meta success
[58499 20210202 14:51:31.836693Z][INFO][lib/super_block.cc:34] read super block success
[58499 20210202 14:51:31.836701Z][INFO][lib/fd_handle.cc:20] max fd num: 400000
[58499 20210202 14:51:31.852179Z][INFO][lib/block_handle.cc:30] set max block num: 31964
[58499 20210202 14:51:31.852198Z][INFO][lib/block_handle.cc:39] shuffle block id supported: true
[58499 20210202 14:51:31.855994Z][WARN][lib/dir_handle.cc:34] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 3438912382
[58499 20210202 14:51:31.878084Z][INFO][lib/dir_handle.cc:69] read directory meta success, free num:99999
[58499 20210202 14:51:31.878103Z][INFO][lib/file_handle.cc:40] file handle: 0 name: 1.log size: 0 dh: 0 seq_no: 0 child_fh: -1 parent_fh: -1
[58499 20210202 14:51:31.888378Z][INFO][lib/file_handle.cc:80] read file meta success, free num:99999
[58499 20210202 14:51:31.888396Z][INFO][lib/file_block_handle.cc:14] total file block num: 108388
[58499 20210202 14:51:31.963230Z][INFO][lib/file_block_handle.cc:102] read file block meta success, free num:108388
[58499 20210202 14:51:31.965966Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[58499 20210202 14:51:31.965982Z][INFO][lib/super_block.cc:173] mount point has been configured
[58499 20210202 14:51:31.965990Z][INFO][lib/file_store_udisk.cc:1051] create fs mount point: /mnt/mysql/data/
[58499 20210202 14:51:31.966003Z][INFO][lib/block_fs_fuse.cc:1513] FUSE version: 3.10.1
[58499 20210202 14:51:31.966027Z][INFO][lib/block_fs_fuse.cc:1525] fuse enable foreground
[58499 20210202 14:51:31.966040Z][INFO][lib/block_fs_fuse.cc:1554] fuse mount_arg: allow_other,umask=0755,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[58499 20210202 14:51:31.966053Z][INFO][lib/block_fs_fuse.cc:1559] argv_cnt: 6
[58514 20210202 14:51:31.991787Z][INFO][lib/block_fs_fuse.cc:945] call bfs_init
[58514 20210202 14:51:31.991835Z][INFO][lib/block_fs_fuse.cc:887] Protocol version: 7.27
[58514 20210202 14:51:31.991839Z][INFO][lib/block_fs_fuse.cc:889] Capabilities:
[58514 20210202 14:51:31.991841Z][INFO][lib/block_fs_fuse.cc:891] FUSE_CAP_WRITEBACK_CACHE
[58514 20210202 14:51:31.991847Z][INFO][lib/block_fs_fuse.cc:892] FUSE_CAP_ASYNC_READ
[58514 20210202 14:51:31.991850Z][INFO][lib/block_fs_fuse.cc:893] FUSE_CAP_POSIX_LOCKS
[58514 20210202 14:51:31.991860Z][INFO][lib/block_fs_fuse.cc:895] FUSE_CAP_ATOMIC_O_TRUNC
[58514 20210202 14:51:31.991862Z][INFO][lib/block_fs_fuse.cc:897] FUSE_CAP_EXPORT_SUPPORT
[58514 20210202 14:51:31.991866Z][INFO][lib/block_fs_fuse.cc:898] FUSE_CAP_DONT_MASK
[58514 20210202 14:51:31.991868Z][INFO][lib/block_fs_fuse.cc:899] FUSE_CAP_SPLICE_MOVE
[58514 20210202 14:51:31.991874Z][INFO][lib/block_fs_fuse.cc:900] FUSE_CAP_SPLICE_READ
[58514 20210202 14:51:31.991876Z][INFO][lib/block_fs_fuse.cc:902] FUSE_CAP_SPLICE_WRITE
[58514 20210202 14:51:31.991879Z][INFO][lib/block_fs_fuse.cc:903] FUSE_CAP_FLOCK_LOCKS
[58514 20210202 14:51:31.991882Z][INFO][lib/block_fs_fuse.cc:904] FUSE_CAP_IOCTL_DIR
[58514 20210202 14:51:31.991884Z][INFO][lib/block_fs_fuse.cc:906] FUSE_CAP_AUTO_INVAL_DATA
[58514 20210202 14:51:31.991887Z][INFO][lib/block_fs_fuse.cc:907] FUSE_CAP_READDIRPLUS
[58514 20210202 14:51:31.991889Z][INFO][lib/block_fs_fuse.cc:909] FUSE_CAP_READDIRPLUS_AUTO
[58514 20210202 14:51:31.991895Z][INFO][lib/block_fs_fuse.cc:910] FUSE_CAP_ASYNC_DIO
[58514 20210202 14:51:31.991897Z][INFO][lib/block_fs_fuse.cc:912] FUSE_CAP_WRITEBACK_CACHE
[58514 20210202 14:51:31.991901Z][INFO][lib/block_fs_fuse.cc:914] FUSE_CAP_NO_OPEN_SUPPORT
[58514 20210202 14:51:31.991903Z][INFO][lib/block_fs_fuse.cc:916] FUSE_CAP_PARALLEL_DIROPS
[58514 20210202 14:51:31.991909Z][INFO][lib/block_fs_fuse.cc:917] FUSE_CAP_POSIX_ACL

[58515 20210202 14:51:37.200378Z][INFO][lib/block_fs_fuse.cc:126] call mfs_getattr file: /
[58514 20210202 14:51:38.902754Z][INFO][lib/block_fs_fuse.cc:126] call mfs_getattr file: /
[58515 20210202 14:51:39.385198Z][INFO][lib/file_handle.cc:694] get created file name: /mnt/mysql/data/
[58515 20210202 14:51:39.385208Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/ mount point: /mnt/mysql/data/
[58515 20210202 14:51:39.385212Z][ERROR][lib/super_block.cc:262] file cannot endwith dir separator: /mnt/mysql/data/
[58514 20210202 14:51:40.925726Z][INFO][lib/block_fs_fuse.cc:768] call bfs_opendir: /
[58514 20210202 14:51:40.925766Z][INFO][lib/file_store_udisk.cc:228] open directory: /mnt/mysql/data/
[58514 20210202 14:51:40.925775Z][INFO][lib/dir_handle.cc:615] open directory /mnt/mysql/data/
[58514 20210202 14:51:40.925787Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[58514 20210202 14:51:40.925793Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[58515 20210202 14:51:40.925942Z][INFO][lib/block_fs_fuse.cc:126] call mfs_getattr file: /
[58514 20210202 14:51:40.926073Z][INFO][lib/block_fs_fuse.cc:808] call bfs_readdir: /
[58514 20210202 14:51:40.926085Z][INFO][lib/block_fs_fuse.cc:817] readdir: /mnt/mysql/data/
[58514 20210202 14:51:40.926089Z][INFO][lib/directory.cc:103] init scan directory: /mnt/mysql/data/
[58514 20210202 14:51:40.926099Z][INFO][lib/directory.cc:123] scan file: 1.log
[58514 20210202 14:51:40.926125Z][INFO][lib/directory.cc:50] 1612248700 2021-02-02 14:51:40
[58515 20210202 14:51:40.926246Z][INFO][lib/block_fs_fuse.cc:126] call mfs_getattr file: /
[58514 20210202 14:51:40.926782Z][INFO][lib/block_fs_fuse.cc:126] call mfs_getattr file: /1.log
[58514 20210202 14:51:40.926790Z][INFO][lib/file_store_udisk.cc:318] stat path: /mnt/mysql/data/1.log
[58514 20210202 14:51:40.926793Z][INFO][lib/file_store_udisk.cc:327] stat path: /mnt/mysql/data/1.log
[58514 20210202 14:51:40.926796Z][INFO][lib/file_handle.cc:694] get created file name: /mnt/mysql/data/1.log
[58514 20210202 14:51:40.926802Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/1.log mount point: /mnt/mysql/data/
[58514 20210202 14:51:40.926811Z][INFO][lib/file_handle.cc:135] transformPath dirname: /mnt/mysql/data/
[58514 20210202 14:51:40.926815Z][INFO][lib/file_handle.cc:136] transformPath filename: 1.log
[58515 20210202 14:51:40.926945Z][INFO][lib/block_fs_fuse.cc:872] call bfs_releasedir: /
[58515 20210202 14:51:40.926952Z][INFO][lib/dir_handle.cc:659] close directory name: /mnt/mysql/data/ fd: 0
```


### 4. 基本命令测试

首先开启FuseEnable的配置：

$ pwd
/home/luotang/blockfs/build
$ sudo ./tool/block_fs_mount

$ ps aux | grep mount
root      488509  0.0  0.0  50132  3920 pts/5    S+   10:01   0:00 sudo ./tool/block_fs_mount
root      488510  0.0  0.9 860940 618076 pts/5   Sl+  10:01   0:00 ./tool/block_fs_mount
root      488523  0.0  0.0   4540   848 ?        Ss   10:01   0:00 fusermount3 -o rw,nosuid,nodev,allow_other,auto_unmount,subtype=block_fs_mount -- /home/luotang/bfs
luotang   558187  0.0  0.0  13144  1092 pts/6    S+   11:45   0:00 grep --color=auto mount

luotang@10-23-227-66:~/bfs$ cat /proc/mounts
hugetlbfs /dev/hugepages hugetlbfs rw,relatime,pagesize=2M 0 0
mqueue /dev/mqueue mqueue rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,relatime 0 0
configfs /sys/kernel/config configfs rw,relatime 0 0
/dev/vdb /data ext4 rw,relatime,discard 0 0
fusectl /sys/fs/fuse/connections fusectl rw,relatime 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,relatime 0 0
tracefs /sys/kernel/debug/tracing tracefs rw,relatime 0 0
tmpfs /run/user/1002 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1002,gid=1002 0 0
tmpfs /run/user/1001 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1001,gid=1001 0 0
tmpfs /run/user/1000 tmpfs rw,nosuid,nodev,relatime,size=6574104k,mode=700,uid=1000,gid=1000 0 0
block_fs_mount /home/luotang/bfs fuse.block_fs_mount rw,nosuid,nodev,relatime,user_id=0,group_id=0,allow_other 0 0



### 5. 启动问题

#### 权限问题
```sh
[30316 20210205 18:15:12.797063Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[30316 20210205 18:15:12.797067Z][INFO][lib/super_block.cc:173] mount point has been configured
[30316 20210205 18:15:12.797069Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[30316 20210205 18:15:12.797078Z][INFO][lib/block_fs_fuse.cc:1513] FUSE version: 3.10.1
[30316 20210205 18:15:12.797092Z][INFO][lib/block_fs_fuse.cc:1525] fuse enable foreground
[30316 20210205 18:15:12.797101Z][INFO][lib/block_fs_fuse.cc:1554] fuse mount_arg: allow_other,umask=0755,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[30316 20210205 18:15:12.797103Z][INFO][lib/block_fs_fuse.cc:1559] argv_cnt: 6
fusermount3: user has no write access to mountpoint /home/luotang/bfs
luotang@10-23-227-66:~/blockfs$ ll /home/luotang/bfs/
total 8
drwxr-xr-x  2 root    root    4096 Jan 19 16:58 ./
drwxr-xr-x 17 luotang luotang 4096 Feb  5 16:30 ../
luotang@10-23-227-66:~/blockfs$ sudo chown -R luotang:luotang /home/luotang/bfs/
luotang@10-23-227-66:~/blockfs$ ll /home/luotang/bfs/
total 8
drwxr-xr-x  2 luotang luotang 4096 Jan 19 16:58 ./
drwxr-xr-x 17 luotang luotang 4096 Feb  5 16:30 ../
```


```sh
[31232 20210205 18:17:34.114725Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[31232 20210205 18:17:34.114728Z][INFO][lib/super_block.cc:173] mount point has been configured
[31232 20210205 18:17:34.114731Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[31232 20210205 18:17:34.114740Z][INFO][lib/block_fs_fuse.cc:1513] FUSE version: 3.10.1
[31232 20210205 18:17:34.114756Z][INFO][lib/block_fs_fuse.cc:1525] fuse enable foreground
[31232 20210205 18:17:34.114764Z][INFO][lib/block_fs_fuse.cc:1554] fuse mount_arg: allow_other,umask=0755,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[31232 20210205 18:17:34.114767Z][INFO][lib/block_fs_fuse.cc:1559] argv_cnt: 6
fusermount3: option allow_other only allowed if 'user_allow_other' is set in /usr/local/etc/fuse.conf
luotang@10-23-227-66:~/blockfs$ ./build/tool/block_fs_mount
```

#### issue

https://github.com/containers/fuse-overlayfs/issues/35#issuecomment-451952680

### luotang@10-23-227-66:~/blockfs$ vim /usr/local/etc/fuse.conf
```ini
# The file /etc/fuse.conf allows for the following parameters:
#
# user_allow_other - Using the allow_other mount option works fine as root, in
# order to have it work as user you need user_allow_other in /etc/fuse.conf as
# well. (This option allows users to use the allow_other option.) You need
# allow_other if you want users other than the owner to access a mounted fuse.
# This option must appear on a line by itself. There is no value, just the
# presence of the option.

user_allow_other


# mount_max = n - this option sets the maximum number of mounts.
# Currently (2014) it must be typed exactly as shown
# (with a single space before and after the equals sign).

#mount_max = 1000

```


```sh
[31813 20210205 18:19:26.861753Z][INFO][lib/file_handle.cc:85] read file meta success, free num:99979
[31813 20210205 18:19:26.861773Z][INFO][lib/file_block_handle.cc:14] total file block num: 108388
[31813 20210205 18:19:26.925318Z][INFO][lib/file_block_handle.cc:102] read file block meta success, free num:108368
[31813 20210205 18:19:26.927990Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[31813 20210205 18:19:26.927993Z][INFO][lib/super_block.cc:173] mount point has been configured
[31813 20210205 18:19:26.927996Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[31813 20210205 18:19:26.928005Z][INFO][lib/block_fs_fuse.cc:1513] FUSE version: 3.10.1
[31813 20210205 18:19:26.928017Z][INFO][lib/block_fs_fuse.cc:1525] fuse enable foreground
[31813 20210205 18:19:26.928026Z][INFO][lib/block_fs_fuse.cc:1554] fuse mount_arg: allow_other,umask=0755,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[31813 20210205 18:19:26.928028Z][INFO][lib/block_fs_fuse.cc:1559] argv_cnt: 6
fusermount3: mount failed: Operation not permitted
luotang@10-23-227-66:~/blockfs$
```


```sh
$ ll $(which fusermount3)
-rwsr-xr-x 1 root trusted 31504 27. Aug 2019  /usr/bin/fusermount3

luotang@10-23-227-66:~/blockfs$ ll /usr/local/bin/fusermount3
-rwxrwxrwx 1 root root 135016 Jan 20 15:31 /usr/local/bin/fusermount3*
luotang@10-23-227-66:~/blockfs$ sudo chown  luotang:luotang /usr/local/bin/fusermount3
luotang@10-23-227-66:~/blockfs$ ll /usr/local/bin/fusermount3
-rwxrwxrwx 1 luotang luotang 135016 Jan 20 15:31 /usr/local/bin/fusermount3*
luotang@10-23-227-66:~/blockfs$

^C
luotang@10-23-227-66:~/blockfs/build/tool$ ls -l /usr/bin/fusermount3 /dev/fuse
crw-rw-rw- 1 luotang luotang 10, 229 Dec 19 11:42 /dev/fuse
-rwxr-xr-x 1 luotang luotang  134832 Feb  1 15:11 /usr/bin/fusermount3
luotang@10-23-227-66:~/blockfs/build/tool$

```

#### fusermount: Operation not permitted (GuixSD)

https://ask.csdn.net/questions/2020647

https://developer.aliyun.com/ask/3108?spm=a2c6h.13159741

## umount

```sh
umount /data/mysql/bfs
```