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

##### 格式化文件系统
```sh
$ sudo ./block_fs_tool -d /dev/vdf -f
[91222 20210202 15:13:26.042194Z][INFO][lib/file_system.cc:23] bfs version: 21.02.02-dc860f7
[91222 20210202 15:13:26.042276Z][WARN][lib/block_device.cc:379] master node need to set nomerges flag
[91222 20210202 15:13:26.042319Z][INFO][lib/block_device.cc:398] set /dev/vdf nomerges 2 successfully
[91222 20210202 15:13:26.042334Z][INFO][lib/block_device.cc:447] open block device /dev/vdf success
[91222 20210202 15:13:26.042338Z][INFO][lib/block_device.cc:214] get block device size: 536870912000 B
[91222 20210202 15:13:26.042341Z][INFO][lib/block_device.cc:234] get block device blk size: 512
[91222 20210202 15:13:26.042748Z][INFO][lib/super_block.cc:145] write all super block success
[91222 20210202 15:13:26.391164Z][INFO][lib/dir_handle.cc:115] write all directory meta success
[91222 20210202 15:13:26.482517Z][INFO][lib/file_handle.cc:113] write all file meta success
[91222 20210202 15:13:27.924556Z][INFO][lib/file_block_handle.cc:137] write all file block meta success
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
[90922 20210202 15:12:42.019836Z][INFO][tool/block_fs_tool.cc:835] dump uuid: 6a4d31e7-9fc7-427d-8054-c8fa7028f8d4
[90922 20210202 15:12:42.019841Z][INFO][tool/block_fs_tool.cc:860] total cost time: 2.241 seconds
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