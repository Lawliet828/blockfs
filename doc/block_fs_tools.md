## 1. block_fs_tool原生工具

###  介绍

该工具主要用于抹盘格式化(做文件系统)，dump元数据的信息等等

###  命令参数

```sh
$ sudo ./build/tool/block_fs_tool -v
[90002 20210202 15:09:50.260890Z][INFO][tool/block_fs_tool.cc:217] Build time    : Feb  2 2021 11:58:46
[90002 20210202 15:09:50.260958Z][INFO][tool/block_fs_tool.cc:218] Build version : 0x1
[90002 20210202 15:09:50.260971Z][INFO][tool/block_fs_tool.cc:219] Run options:
[90002 20210202 15:09:50.260975Z][INFO][tool/block_fs_tool.cc:220]  -d, --device   Device such as: /dev/vdb
[90002 20210202 15:09:50.260980Z][INFO][tool/block_fs_tool.cc:221]  -f, --format   Format device.
[90002 20210202 15:09:50.261012Z][INFO][tool/block_fs_tool.cc:222]  -p, --dump     Dump device, xxx args.
[90002 20210202 15:09:50.261019Z][INFO][tool/block_fs_tool.cc:223]  -s, --simu     Mount point path.
[90002 20210202 15:09:50.261026Z][INFO][tool/block_fs_tool.cc:224]  -c, --check    Check format whether success.
[90002 20210202 15:09:50.261030Z][INFO][tool/block_fs_tool.cc:225]  -m, --meta     Dump file meta content.
[90002 20210202 15:09:50.261034Z][INFO][tool/block_fs_tool.cc:226]  -b, --blk      Export blk device content.
[90002 20210202 15:09:50.261036Z][INFO][tool/block_fs_tool.cc:227]  -v, --version  Print the version.
[90002 20210202 15:09:50.261038Z][INFO][tool/block_fs_tool.cc:228]  -h, --help     Print help info.
[90002 20210202 15:09:50.261044Z][INFO][tool/block_fs_tool.cc:230] Examples:
[90002 20210202 15:09:50.261046Z][INFO][tool/block_fs_tool.cc:231]  help    : ./block_fs_tool -h
[90002 20210202 15:09:50.261049Z][INFO][tool/block_fs_tool.cc:232]  version : ./block_fs_tool -v
[90002 20210202 15:09:50.261051Z][INFO][tool/block_fs_tool.cc:233]  format  : ./block_fs_tool -d /dev/vdb -f
[90002 20210202 15:09:50.261053Z][INFO][tool/block_fs_tool.cc:234]  format  : ./block_fs_tool -d /dev/vdb --format
[90002 20210202 15:09:50.261056Z][INFO][tool/block_fs_tool.cc:235]  dump    : ./block_fs_tool -d /dev/vdb -p check
[90002 20210202 15:09:50.261059Z][INFO][tool/block_fs_tool.cc:236]  dump    : ./block_fs_tool -d /dev/vdb -p negot
[90002 20210202 15:09:50.261063Z][INFO][tool/block_fs_tool.cc:237]  dump    : ./block_fs_tool -d /dev/vdb -p super
[90002 20210202 15:09:50.261071Z][INFO][tool/block_fs_tool.cc:238]  dump    : ./block_fs_tool -d /dev/vdb -p uuid
[90002 20210202 15:09:50.261075Z][INFO][tool/block_fs_tool.cc:239]  dump    : ./block_fs_tool -d /dev/vdb -p dirs
[90002 20210202 15:09:50.261081Z][INFO][tool/block_fs_tool.cc:240]  dump    : ./block_fs_tool -d /dev/vdb -p files
[90002 20210202 15:09:50.261084Z][INFO][tool/block_fs_tool.cc:241]  dump    : ./block_fs_tool -d /dev/vdb -p all
[90002 20210202 15:09:50.261087Z][INFO][tool/block_fs_tool.cc:242]  meta    : ./block_fs_tool -d /dev/vdb -m file_name
[90002 20210202 15:09:50.261092Z][INFO][tool/block_fs_tool.cc:243]  dump    : ./block_fs_tool -d /dev/vdb --dump xxx
[90002 20210202 15:09:50.261095Z][INFO][tool/block_fs_tool.cc:244]  blk     : ./block_fs_tool -d /dev/vdb -b offset:size
[90002 20210202 15:09:50.261100Z][INFO][tool/block_fs_tool.cc:245]  cat     : ./block_fs_tool -d /dev/vdb -c /mnt/mysql/data/1.ibt
[90002 20210202 15:09:50.261104Z][INFO][tool/block_fs_tool.cc:247]  simu    : ./block_fs_tool -d /dev/vdb -s /mnt/mysql/data/
[90002 20210202 15:09:50.261121Z][INFO][tool/block_fs_tool.cc:248]  simu    : [ls,pwd,cd,mkdir,stat,cat,echo,rename,rmdir,touch,rm,unlink]
```


### 使用demo

##### 格式化文件系统
```sh
$ sudo ./block_fs_tool -d /dev/vdf -f
[91222 20210202 15:13:26.042194Z][INFO][lib/file_system.cc:23] bfs version: 21.02.02-dc860f7
[91222 20210202 15:13:26.042276Z][WARN][lib/block_device.cc:379] master node need to set nomerges flag
[91222 20210202 15:13:26.042319Z][INFO][lib/block_device.cc:398] set /dev/vdf nomerges 2 successfully
[91222 20210202 15:13:26.042334Z][INFO][lib/block_device.cc:447] open block device /dev/vdf success
[91222 20210202 15:13:26.042338Z][INFO][lib/block_device.cc:214] get block device size: 536870912000 B
[91222 20210202 15:13:26.042341Z][INFO][lib/block_device.cc:234] get block device blk size: 512
[91222 20210202 15:13:26.042651Z][INFO][lib/negotiation.cc:40] write all negotiation success
[91222 20210202 15:13:26.042748Z][INFO][lib/super_block.cc:145] write all super block success
[91222 20210202 15:13:26.391164Z][INFO][lib/dir_handle.cc:115] write all directory meta success
[91222 20210202 15:13:26.482517Z][INFO][lib/file_handle.cc:113] write all file meta success
[91222 20210202 15:13:27.924556Z][INFO][lib/file_block_handle.cc:137] write all file block meta success
[91222 20210202 15:13:27.951838Z][INFO][lib/journal_handle.cc:77] format journal num: 4096
[91222 20210202 15:13:27.997450Z][INFO][lib/journal_handle.cc:98] write all journal success
[91222 20210202 15:13:27.997488Z][INFO][tool/block_fs_tool.cc:820] format block fs success
[91222 20210202 15:13:27.997497Z][INFO][tool/block_fs_tool.cc:860] total cost time: 1.955 seconds
```

##### 打印uuid
```
$ sudo ./block_fs_tool -d /dev/vdf -p uuid
[90922 20210202 15:12:39.778375Z][INFO][tool/block_fs_tool.cc:149] dump flag: uuid
[90922 20210202 15:12:39.778458Z][INFO][lib/file_system.cc:23] bfs version: 21.02.02-dc860f7
[90922 20210202 15:12:39.778484Z][DEBUG][lib/crc32-x86.cc:145] SSE42 support
[90922 20210202 15:12:39.778518Z][DEBUG][lib/comm_utils.cc:80] coredump rlim_cur: 18446744073709551615 max rlim_max: 18446744073709551615
[90922 20210202 15:12:39.778544Z][WARN][lib/block_device.cc:379] master node need to set nomerges flag
[90922 20210202 15:12:39.778591Z][INFO][lib/block_device.cc:398] set /dev/vdf nomerges 2 successfully
[90922 20210202 15:12:39.778609Z][INFO][lib/block_device.cc:447] open block device /dev/vdf success
[90922 20210202 15:12:39.778615Z][INFO][lib/block_device.cc:214] get block device size: 536870912000 B
[90922 20210202 15:12:39.778618Z][INFO][lib/block_device.cc:234] get block device blk size: 512
[90922 20210202 15:12:39.778838Z][INFO][lib/shm_manager.cc:59] read super block success
[90922 20210202 15:12:39.778846Z][INFO][lib/shm_manager.cc:60] mount point: /mnt/mysql/data/
[90922 20210202 15:12:39.778848Z][INFO][lib/shm_manager.cc:61] block_data_start_offset: 603979776
[90922 20210202 15:12:39.778851Z][INFO][lib/shm_manager.cc:66] shm name: blockfs_shm_6a4d31e7-9fc7-427d-8054-c8fa7028f8d4
[90922 20210202 15:12:39.778856Z][INFO][lib/shm_manager.cc:67] shm size: 603979776
[90922 20210202 15:12:39.778861Z][INFO][lib/shm_manager.cc:72] let's look mem usage:
[90922 20210202 15:12:39.778864Z][INFO][lib/shm_manager.cc:73] totalram: 67318837248
[90922 20210202 15:12:39.778866Z][INFO][lib/shm_manager.cc:74] freeram: 6799577088
[90922 20210202 15:12:39.778869Z][INFO][lib/shm_manager.cc:75] sharedram: 10539008
[90922 20210202 15:12:39.778883Z][INFO][lib/shm_manager.cc:76] need shm: 603979776
[90922 20210202 15:12:39.778890Z][INFO][lib/shm_manager.cc:205] using posix mem aligned meta
[90922 20210202 15:12:41.889680Z][INFO][lib/shm_manager.cc:228] read all meta into shm success
[90922 20210202 15:12:41.889729Z][INFO][lib/shm_manager.cc:263] init and register meta success
[90922 20210202 15:12:41.889741Z][INFO][lib/negotiation.cc:20] read negotiation success
[90922 20210202 15:12:41.889746Z][INFO][lib/super_block.cc:34] read super block success
[90922 20210202 15:12:41.889758Z][INFO][lib/fd_handle.cc:20] max fd num: 400000
[90922 20210202 15:12:41.904908Z][INFO][lib/block_handle.cc:30] set max block num: 31964
[90922 20210202 15:12:41.904927Z][INFO][lib/block_handle.cc:39] shuffle block id supported: true
[90922 20210202 15:12:41.908678Z][WARN][lib/dir_handle.cc:34] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 2647270868
[90922 20210202 15:12:41.908704Z][WARN][lib/dir_handle.cc:34] directory handle: 1 name: /mnt/mysql/data/hahaha/ seq_no: 0 crc: 3010015852
[90922 20210202 15:12:41.930977Z][INFO][lib/dir_handle.cc:62] add child directory: /mnt/mysql/data/hahaha/
[90922 20210202 15:12:41.931002Z][INFO][lib/dir_handle.cc:69] read directory meta success, free num:99998
[90922 20210202 15:12:41.931008Z][INFO][lib/file_handle.cc:40] file handle: 0 name: 1.log size: 0 dh: 0 seq_no: 0 child_fh: -1 parent_fh: -1
[90922 20210202 15:12:41.931031Z][DEBUG][lib/directory.cc:172] add fh: 0 name: 1.log to /mnt/mysql/data/
[90922 20210202 15:12:41.931036Z][INFO][lib/file_handle.cc:40] file handle: 1 name: libblock_fs.a size: 29232290 dh: 0 seq_no: 0 child_fh: -1 parent_fh: -1
[90922 20210202 15:12:41.931040Z][DEBUG][lib/directory.cc:172] add fh: 1 name: libblock_fs.a to /mnt/mysql/data/
[90922 20210202 15:12:41.941749Z][INFO][lib/file_handle.cc:80] read file meta success, free num:99998
[90922 20210202 15:12:41.941770Z][INFO][lib/file_block_handle.cc:14] total file block num: 108388
[90922 20210202 15:12:41.941775Z][DEBUG][lib/file_block_handle.cc:73] libblock_fs.a block index: 0 block id: 24489
[90922 20210202 15:12:41.941779Z][DEBUG][lib/file_block_handle.cc:73] libblock_fs.a block index: 1 block id: 13206
[90922 20210202 15:12:42.017076Z][INFO][lib/file_block_handle.cc:102] read file block meta success, free num:108387
[90922 20210202 15:12:42.019811Z][WARN][lib/journal_handle.cc:61] load journal success, journal head_: -1 journal tail_: -1 min_seq_no_: 0 max_seq_no_: 0 available_seq_no_: 1
[90922 20210202 15:12:42.019830Z][ERROR][lib/journal_handle.cc:104] I am not master, cannot replay journal
[90922 20210202 15:12:42.019836Z][INFO][tool/block_fs_tool.cc:835] dump uuid: 6a4d31e7-9fc7-427d-8054-c8fa7028f8d4
[90922 20210202 15:12:42.019841Z][INFO][tool/block_fs_tool.cc:860] total cost time: 2.241 seconds
```


### 9. block_fs_touch工具
```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_touch -m -n /mnt/mysql/data/sbtest_file
[863075 20210315 14:17:16.098775Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[863075 20210315 14:17:16.211707Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[863075 20210315 14:17:16.211733Z][INFO][lib/super_block.cc:173] mount point has been configured
[863075 20210315 14:17:16.211736Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[863075 20210315 14:17:16.211834Z][INFO][lib/file_handle.cc:440] create file: /mnt/mysql/data/sbtest_file
[863075 20210315 14:17:16.211844Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest_file mount point: /mnt/mysql/data/
[863075 20210315 14:17:16.211850Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[863075 20210315 14:17:16.211854Z][INFO][lib/file_handle.cc:141] transformPath filename: sbtest_file
[863075 20210315 14:17:16.211857Z][INFO][lib/file_handle.cc:196] create new file handle: 2 name: sbtest_file
[863075 20210315 14:17:16.211865Z][INFO][lib/file.cc:29] write file meta, name: sbtest_file fh: 2 align_index: 0 crc: 2573234313
[863081 20210315 14:17:16.211912Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[863081 20210315 14:17:16.211977Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[863081 20210315 14:17:16.211996Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[863081 20210315 14:17:16.212004Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[863075 20210315 14:17:16.212025Z][INFO][lib/file_handle.cc:481] create file success: sbtest_file
[863075 20210315 14:17:16.212034Z][INFO][tool/block_fs_touch.cc:72] create file: /mnt/mysql/data/sbtest_file success
luotang@10-23-227-66:~/blockfs$
```


#### block_fs_unlink工具
```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_unlink -n /mnt/mysql/data/sbtest_file -m
[873268 20210315 15:23:37.914516Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[873268 20210315 15:23:38.009363Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[873268 20210315 15:23:38.009402Z][INFO][lib/super_block.cc:173] mount point has been configured
[873268 20210315 15:23:38.009405Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[873268 20210315 15:23:38.009489Z][INFO][lib/file_store_udisk.cc:779] remove path: /mnt/mysql/data/sbtest_file
[873268 20210315 15:23:38.009498Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/sbtest_file
[873268 20210315 15:23:38.009521Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest_file mount point: /mnt/mysql/data/
[873268 20210315 15:23:38.009534Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[873268 20210315 15:23:38.009540Z][INFO][lib/file_handle.cc:141] transformPath filename: sbtest_file
[873268 20210315 15:23:38.009569Z][INFO][lib/file_handle.cc:604] unlink file: /mnt/mysql/data/sbtest_file
[873268 20210315 15:23:38.009579Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest_file/ mount point: /mnt/mysql/data/[873272 20210315 15:23:38.009526Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[873272 20210315 15:23:38.009608Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground

[873268 20210315 15:23:38.009618Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/data/sbtest_file/
[873268 20210315 15:23:38.009621Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest_file mount point: /mnt/mysql/data/
[873268 20210315 15:23:38.009657Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[873268 20210315 15:23:38.009660Z][INFO][lib/file_handle.cc:141] transformPath filename: sbtest_file
[873272 20210315 15:23:38.009622Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[873272 20210315 15:23:38.009676Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[873268 20210315 15:23:38.009691Z][INFO][lib/file.cc:44] clear file name: sbtest_file fh: 2
[873268 20210315 15:23:38.009701Z][INFO][lib/file.cc:218] release file name: sbtest_file fh: 2 seq_no: 0
[873268 20210315 15:23:38.009706Z][INFO][lib/file.cc:29] write file meta, name: sbtest_file fh: 2 align_index: 0 crc: 3549018922
fuse: bad mount point `/home/luotang/bfs/': Transport endpoint is not connected
[873268 20210315 15:23:38.009866Z][INFO][lib/file_handle.cc:663] remove file success: /mnt/mysql/data/sbtest_file
[873268 20210315 15:23:38.009880Z][INFO][tool/block_fs_unlink.cc:72] rm file: /mnt/mysql/data/sbtest_file success
```


#### block_fs_ls工具
```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_ls -m -n /mnt/mysql/data
[891591 20210315 16:40:25.662723Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[891591 20210315 16:40:25.662820Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[891591 20210315 16:40:25.753179Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[891591 20210315 16:40:25.753201Z][INFO][lib/super_block.cc:173] mount point has been configured
[891591 20210315 16:40:25.753204Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[891591 20210315 16:40:25.753275Z][INFO][lib/file_store_udisk.cc:229] open directory: /mnt/mysql/data
[891591 20210315 16:40:25.753280Z][INFO][lib/dir_handle.cc:604] open directory /mnt/mysql/data
[891591 20210315 16:40:25.753284Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[891591 20210315 16:40:25.753287Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
total 0
[891591 20210315 16:40:25.753305Z][INFO][lib/directory.cc:107] init scan directory: /mnt/mysql/data/
[891591 20210315 16:40:25.753311Z][INFO][lib/directory.cc:109] scan directory: /mnt/mysql/data/test_dir2/
[891591 20210315 16:40:25.753318Z][INFO][lib/directory.cc:127] scan file: test_file2
[891591 20210315 16:40:25.753325Z][INFO][lib/directory.cc:127] scan file: block_fs_tool
[891591 20210315 16:40:25.753339Z][INFO][lib/directory.cc:127] scan file: do_cmake.sh
drwxrwxr-x 0 root root        0 Mar 15 15:45 test_dir2/
-rwxrwxr-x 0 root root        0 Mar 15 15:50 test_file2
-rwxrwxr-x 0 root root  8444752 Mar 12 12:01 block_fs_tool
-rwxrwxr-x 0 root root      286 Mar 12 13:16 do_cmake.sh
[891591 20210315 16:40:25.753390Z][INFO][lib/dir_handle.cc:654] close directory name: /mnt/mysql/data/ fd: 0
[891592 20210315 16:40:25.753360Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1luotang@10-23-227-66:~/blockfs$
```


###  block_fs_extract_device

```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_extract_device -d /dev/vdf -o 10 -l 100000 -n /home/luotang/test.100
[971662 20210315 22:45:14.423608Z][INFO][tool/block_fs_extract_device.cc:86] export: /home/luotang/test.100 success
[971662 20210315 22:45:14.423684Z][INFO][tool/block_fs_extract_device.cc:152] export dev /dev/vdf success


luotang@10-23-227-66:~$ ls -l
total 128
drwxr-xr-x  2 luotang luotang   4096 Jan 19 16:58 bfs
drwxrwxr-x 12 luotang luotang   4096 Mar 15 15:52 blockfs
drwxrwxr-x 12 luotang luotang   4096 Mar 12 00:36 blockfs_stable
drwxrwxr-x 12 luotang luotang   4096 Dec 23 09:49 blockfs_tools
drwxrwxr-x  5 luotang luotang   4096 Feb  3 10:19 deps
-rwxrwxrwx  1 root    root       286 Mar 15 18:09 do_cmake.sh
-rwxrwxrwx  1 root    root       100 Mar 15 20:19 do_cmake.sh.bak
-rwxr-xr-x  1 root    root    100010 Mar 15 22:45 test.100
luotang@10-23-227-66:~$
```