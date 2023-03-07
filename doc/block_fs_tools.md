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
````


## 2. block_fs_cat工具

```sh
$ sudo ./build/tool/block_fs_cat -n /mnt/mysql/data/do_cmake.sh
[872838 20210315 15:21:47.995488Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[872838 20210315 15:21:48.097284Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[872838 20210315 15:21:48.097326Z][INFO][lib/super_block.cc:173] mount point has been configured
[872838 20210315 15:21:48.097329Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[872838 20210315 15:21:48.097454Z][INFO][lib/file_store_udisk.cc:447] open file: /mnt/mysql/data/do_cmake.sh
[872838 20210315 15:21:48.097473Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh/ mount point: /mnt/mysql/data/
[872838 20210315 15:21:48.097478Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/data/do_cmake.sh/
[872838 20210315 15:21:48.097483Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/do_cmake.sh
[872838 20210315 15:21:48.097498Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh mount point: /mnt/mysql/data/
[872838 20210315 15:21:48.097514Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[872838 20210315 15:21:48.097517Z][INFO][lib/file_handle.cc:141] transformPath filename: do_cmake.sh
[872838 20210315 15:21:48.097522Z][WARN][lib/file_handle.cc:861] mock return success, slave just open file: /mnt/mysql/data/do_cmake.sh
[872838 20210315 15:21:48.097527Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[872838 20210315 15:21:48.097531Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[872838 20210315 15:21:48.097533Z][INFO][lib/file_handle.cc:873] open file: /mnt/mysql/data/do_cmake.sh fd: 0 fh: 1
[872842 20210315 15:21:48.097552Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[872842 20210315 15:21:48.097625Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[872838 20210315 15:21:48.097604Z][INFO][lib/file_store_udisk.cc:619] read file fd: 0 len: 286
[872842 20210315 15:21:48.097657Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[872842 20210315 15:21:48.097662Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[872838 20210315 15:21:48.097684Z][INFO][lib/file.cc:791] file name: do_cmake.sh file size: 286 read size: 286 offset: 0
[872838 20210315 15:21:48.097720Z][INFO][lib/file.cc:866] current offset: 0 file_block_index: 0 block_index_in_file_block: 0 block_offset_in_block: 0
[872838 20210315 15:21:48.097731Z][INFO][lib/file.cc:279] do_cmake.sh get file block cut: 0 file block size: 1
[872838 20210315 15:21:48.097735Z][INFO][lib/file.cc:919] do_cmake.sh data_start_offset: 603979776 need read offset: 0 read block index: 1467 read block offset: 0 block_read_size: 286 udisk_offset: 25216155648
[872838 20210315 15:21:48.097746Z][INFO][lib/file.cc:946] do_cmake.sh read udisk offset: 25216155648 read size: 286 buffer addr: 0x140734019031728
[872838 20210315 15:21:48.097971Z][INFO][lib/comm_utils.cc:95] -----------------begin-------------------

[872838 20210315 15:21:48.097982Z][INFO][lib/comm_utils.cc:109] 23 21 2f 62 69 6e 2f 62 61 73 68 0a 69 66 20 5b 
[872838 20210315 15:21:48.097986Z][INFO][lib/comm_utils.cc:109] 5b 20 24 23 20 3e 20 31 20 5d 5d 3b 20 74 68 65 
[872838 20210315 15:21:48.097992Z][INFO][lib/comm_utils.cc:109] 6e 0a 20 20 20 20 65 63 68 6f 20 22 75 73 61 67 
[872838 20210315 15:21:48.097996Z][INFO][lib/comm_utils.cc:109] 65 20 24 30 22 0a 20 20 20 20 65 78 69 74 20 31 
[872838 20210315 15:21:48.098000Z][INFO][lib/comm_utils.cc:109] 0a 66 69 0a 0a 69 66 20 74 65 73 74 20 2d 65 20 
[872838 20210315 15:21:48.098004Z][INFO][lib/comm_utils.cc:109] 62 75 69 6c 64 3b 74 68 65 6e 0a 20 20 20 20 65 
[872838 20210315 15:21:48.098011Z][INFO][lib/comm_utils.cc:109] 63 68 6f 20 22 2d 2d 20 62 75 69 6c 64 20 64 69 
[872838 20210315 15:21:48.098015Z][INFO][lib/comm_utils.cc:109] 72 20 61 6c 72 65 61 64 79 20 65 78 69 73 74 73 
[872838 20210315 15:21:48.098020Z][INFO][lib/comm_utils.cc:109] 3b 20 72 6d 20 2d 72 66 20 62 75 69 6c 64 20 61 
[872838 20210315 15:21:48.098023Z][INFO][lib/comm_utils.cc:109] 6e 64 20 72 65 2d 72 75 6e 22 0a 20 20 20 20 72 
[872838 20210315 15:21:48.098033Z][INFO][lib/comm_utils.cc:109] 6d 20 2d 72 66 20 62 75 69 6c 64 0a 66 69 0a 0a 
[872838 20210315 15:21:48.098038Z][INFO][lib/comm_utils.cc:109] 6d 6b 64 69 72 20 62 75 69 6c 64 0a 63 64 20 62 
[872838 20210315 15:21:48.098047Z][INFO][lib/comm_utils.cc:109] 75 69 6c 64 0a 0a 23 63 6d 61 6b 65 20 2d 44 43 
[872838 20210315 15:21:48.098051Z][INFO][lib/comm_utils.cc:109] 4d 41 4b 45 5f 42 55 49 4c 44 5f 54 59 50 45 3d 
[872838 20210315 15:21:48.098057Z][INFO][lib/comm_utils.cc:109] 44 65 62 75 67 20 22 24 40 22 20 2e 2e 0a 63 6d 
[872838 20210315 15:21:48.098061Z][INFO][lib/comm_utils.cc:109] 61 6b 65 20 2d 44 43 4d 41 4b 45 5f 42 55 49 4c 
[872838 20210315 15:21:48.098068Z][INFO][lib/comm_utils.cc:109] 44 5f 54 59 50 45 3d 52 65 6c 57 69 74 68 44 65 
[872838 20210315 15:21:48.098076Z][INFO][lib/comm_utils.cc:115] 62 49 6e 66 6f 20 22 24 40 22 20 2e 2e 0a 
[872838 20210315 15:21:48.098079Z][INFO][lib/comm_utils.cc:117] -----------------end-------------------

[872838 20210315 15:21:48.098083Z][INFO][tool/block_fs_cat.cc:107] cat file: /mnt/mysql/data/do_cmake.sh success
luotang@10-23-227-66:~/blockfs$ 
```


## 3. block_fs_mv工具
```
luotang@10-23-227-66:~/blockfs$ sudo /home/luotang/blockfs/build/tool/block_fs_touch -m -n /mnt/mysql/data/test_file
[881333 20210315 15:50:40.847115Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[881333 20210315 15:50:40.847236Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[881333 20210315 15:50:40.938375Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[881333 20210315 15:50:40.938395Z][INFO][lib/super_block.cc:173] mount point has been configured
[881333 20210315 15:50:40.938398Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[881333 20210315 15:50:40.938492Z][INFO][lib/file_handle.cc:440] create file: /mnt/mysql/data/test_file
[881333 20210315 15:50:40.938508Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_file mount point: /mnt/mysql/data/
[881333 20210315 15:50:40.938514Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[881333 20210315 15:50:40.938518Z][INFO][lib/file_handle.cc:141] transformPath filename: test_file
[881333 20210315 15:50:40.938527Z][INFO][lib/file_handle.cc:196] create new file handle: 2 name: test_file
[881333 20210315 15:50:40.938539Z][INFO][lib/file.cc:29] write file meta, name: test_file fh: 2 align_index: 0 crc: 2289579665
[881337 20210315 15:50:40.938567Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[881337 20210315 15:50:40.938627Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[881337 20210315 15:50:40.938639Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[881337 20210315 15:50:40.938645Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[881333 20210315 15:50:40.938690Z][INFO][lib/file_handle.cc:481] create file success: test_file
[881333 20210315 15:50:40.938716Z][INFO][tool/block_fs_touch.cc:72] create file: /mnt/mysql/data/test_file success
fuse: bad mount point `/home/luotang/bfs/': Transport endpoint is not connected
luotang@10-23-227-66:~/blockfs$ 
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_mv -m -s /mnt/mysql/data/test_file -t /mnt/mysql/data/test_file2
[881389 20210315 15:51:08.441104Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[881389 20210315 15:51:08.441231Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[881389 20210315 15:51:08.532927Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[881389 20210315 15:51:08.532949Z][INFO][lib/super_block.cc:173] mount point has been configured
[881389 20210315 15:51:08.532951Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[881389 20210315 15:51:08.533028Z][INFO][lib/file_store_udisk.cc:499] old path: /mnt/mysql/data/test_file newpath: /mnt/mysql/data/test_file2
[881389 20210315 15:51:08.533038Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_file
[881389 20210315 15:51:08.533043Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_file mount point: /mnt/mysql/data/
[881389 20210315 15:51:08.533050Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[881389 20210315 15:51:08.533053Z][INFO][lib/file_handle.cc:141] transformPath filename: test_file
[881389 20210315 15:51:08.533060Z][INFO][lib/file.cc:139] rename old name: test_file to: /mnt/mysql/data/test_file2
[881389 20210315 15:51:08.533098Z][INFO][lib/file.cc:29] write file meta, name: test_file2 fh: 2 align_index: 0 crc: 3971219673
[881395 20210315 15:51:08.533105Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[881395 20210315 15:51:08.533177Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[881395 20210315 15:51:08.533189Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[881395 20210315 15:51:08.533192Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
fuse: bad mount point `/home/luotang/bfs/': Transport endpoint is not connected
[881389 20210315 15:51:08.533252Z][INFO][tool/block_fs_mv.cc:81] mv file: /mnt/mysql/data/test_file to /mnt/mysql/data/test_file2 success
luotang@10-23-227-66:~/blockfs$ 
```

## 4. block_fs_rename工具
```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_rename -m -s /mnt/mysql/data/test_dir -t /mnt/mysql/data/test_dir2
[880973 20210315 15:49:04.658555Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[880973 20210315 15:49:04.658683Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir/ seq_no: 0 crc: 3346508364
[880973 20210315 15:49:04.757515Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[880973 20210315 15:49:04.757552Z][INFO][lib/super_block.cc:173] mount point has been configured
[880973 20210315 15:49:04.757555Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[880973 20210315 15:49:04.757656Z][INFO][lib/file_store_udisk.cc:499] old path: /mnt/mysql/data/test_dir newpath: /mnt/mysql/data/test_dir2
[880973 20210315 15:49:04.757666Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_dir
[880973 20210315 15:49:04.757673Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir mount point: /mnt/mysql/data/
[880973 20210315 15:49:04.757682Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[880973 20210315 15:49:04.757685Z][INFO][lib/file_handle.cc:141] transformPath filename: test_dir
[880973 20210315 15:49:04.757690Z][WARN][lib/file_handle.cc:725] file not exist: test_dir
[880973 20210315 15:49:04.757698Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir/ mount point: /mnt/mysql/data/
[880973 20210315 15:49:04.757702Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_dir2
[880973 20210315 15:49:04.757708Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2 mount point: /mnt/mysql/data/
[880973 20210315 15:49:04.757776Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[880973 20210315 15:49:04.757780Z][INFO][lib/file_handle.cc:141] transformPath filename: test_dir2
[880973 20210315 15:49:04.757792Z][WARN][lib/file_handle.cc:725] file not exist: test_dir2
[880979 20210315 15:49:04.757696Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[880979 20210315 15:49:04.757820Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[880973 20210315 15:49:04.757797Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir/ mount point: /mnt/mysql/data/
[880973 20210315 15:49:04.757828Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/ mount point: /mnt/mysql/data/
[880979 20210315 15:49:04.757830Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[880979 20210315 15:49:04.757857Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[880973 20210315 15:49:04.757864Z][INFO][lib/directory.cc:236] rename old dir: /mnt/mysql/data/test_dir/ to: /mnt/mysql/data/test_dir2/
[880973 20210315 15:49:04.757876Z][INFO][lib/directory.cc:147] directory handle: 1 align_index: 0
fuse: bad mount point `/home/luotang/bfs/': Transport endpoint is not connected
```

## 5. block_fs_cp工具

分为导入，导出, 内部拷贝三种场景, 区别在于路径的不同

```sh
BFS向本地系统拷贝

luotang@10-23-227-66:~$ sudo ./blockfs/build/tool/block_fs_cp -m -s /mnt/mysql/data/do_cmake.sh -t /home/luotang/do_cmake.sh
[915549 20210315 18:09:07.432360Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[915549 20210315 18:09:07.432496Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[915549 20210315 18:09:07.543129Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[915549 20210315 18:09:07.543158Z][INFO][lib/super_block.cc:173] mount point has been configured
[915549 20210315 18:09:07.543163Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[915549 20210315 18:09:07.543263Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh mount point: /mnt/mysql/data/
[915549 20210315 18:09:07.543275Z][INFO][lib/super_block.cc:235] check target path: /home/luotang/do_cmake.sh mount point: /mnt/mysql/data/
[915549 20210315 18:09:07.543280Z][ERROR][lib/super_block.cc:246] path not startwith mount prefix: /home/luotang/do_cmake.sh
[915549 20210315 18:09:07.543296Z][INFO][lib/file_store_udisk.cc:447] open file: /mnt/mysql/data/do_cmake.sh
[915549 20210315 18:09:07.543312Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh/ mount point: /mnt/mysql/data/
[915549 20210315 18:09:07.543319Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/data/do_cmake.sh/
[915549 20210315 18:09:07.543350Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/do_cmake.sh
[915549 20210315 18:09:07.543374Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh mount point: /mnt/mysql/data/
[915549 20210315 18:09:07.543383Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[915549 20210315 18:09:07.543388Z][INFO][lib/file_handle.cc:141] transformPath filename: do_cmake.sh
[915549 20210315 18:09:07.543396Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[915549 20210315 18:09:07.543402Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[915549 20210315 18:09:07.543408Z][INFO][lib/file_handle.cc:873] open file: /mnt/mysql/data/do_cmake.sh fd: 0 fh: 1
[915553 20210315 18:09:07.543374Z][INFO][lib/block_fs_fuse.cc:1523] FUSE version: 3.10.1
[915553 20210315 18:09:07.543441Z][INFO][lib/block_fs_fuse.cc:1535] fuse enable foreground
[915553 20210315 18:09:07.543454Z][INFO][lib/block_fs_fuse.cc:1575] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[915553 20210315 18:09:07.543463Z][INFO][lib/block_fs_fuse.cc:1580] argv_cnt: 6
[915549 20210315 18:09:07.543497Z][INFO][lib/file_store_udisk.cc:619] read file fd: 0 len: 286
[915549 20210315 18:09:07.543520Z][INFO][lib/file.cc:791] file name: do_cmake.sh file size: 286 read size: 286 offset: 0
[915549 20210315 18:09:07.543541Z][INFO][lib/file.cc:866] current offset: 0 file_block_index: 0 block_index_in_file_block: 0 block_offset_in_block: 0
[915549 20210315 18:09:07.543554Z][INFO][lib/file.cc:279] do_cmake.sh get file block cut: 0 file block size: 1
[915549 20210315 18:09:07.543561Z][INFO][lib/file.cc:919] do_cmake.sh data_start_offset: 603979776 need read offset: 0 read block index: 1467 read block offset: 0 block_read_size: 286 udisk_offset: 25216155648
[915549 20210315 18:09:07.543569Z][INFO][lib/file.cc:946] do_cmake.sh read udisk offset: 25216155648 read size: 286 buffer addr: 0x94287401242624
[915549 20210315 18:09:07.554363Z][INFO][tool/block_fs_cp.cc:148] copy: /mnt/mysql/data/do_cmake.sh to /home/luotang/do_cmake.sh success
[915549 20210315 18:09:07.554433Z][INFO][tool/block_fs_cp.cc:254] copy file: /mnt/mysql/data/do_cmake.sh to /home/luotang/do_cmake.sh success
luotang@10-23-227-66:~$ 
```

```sh
本地系统向BFS拷贝

luotang@10-23-227-66:~$ sudo ./blockfs/build/tool/block_fs_cp -m -s /home/luotang/do_cmake.sh -t /mnt/mysql/data/test_dir2/do_cmake.sh2
[916202 20210315 18:11:50.226780Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[916202 20210315 18:11:50.226895Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[916202 20210315 18:11:50.339088Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[916202 20210315 18:11:50.339119Z][INFO][lib/super_block.cc:173] mount point has been configured
[916202 20210315 18:11:50.339121Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339197Z][INFO][lib/super_block.cc:235] check target path: /home/luotang/do_cmake.sh mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339204Z][ERROR][lib/super_block.cc:246] path not startwith mount prefix: /home/luotang/do_cmake.sh
[916202 20210315 18:11:50.339210Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/do_cmake.sh2 mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339233Z][INFO][lib/file_store_udisk.cc:447] open file: /mnt/mysql/data/test_dir2/do_cmake.sh2
[916202 20210315 18:11:50.339246Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/do_cmake.sh2/ mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339248Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/data/test_dir2/do_cmake.sh2/
[916202 20210315 18:11:50.339257Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_dir2/do_cmake.sh2
[916202 20210315 18:11:50.339264Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/do_cmake.sh2 mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339269Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/test_dir2/
[916202 20210315 18:11:50.339276Z][INFO][lib/file_handle.cc:141] transformPath filename: do_cmake.sh2
[916202 20210315 18:11:50.339278Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/ mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339285Z][WARN][lib/file_handle.cc:725] file not exist: do_cmake.sh2
[916202 20210315 18:11:50.339302Z][INFO][lib/file_handle.cc:491] create file: /mnt/mysql/data/test_dir2/do_cmake.sh2
[916202 20210315 18:11:50.339309Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/do_cmake.sh2 mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339316Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/test_dir2/
[916202 20210315 18:11:50.339319Z][INFO][lib/file_handle.cc:141] transformPath filename: do_cmake.sh2
[916202 20210315 18:11:50.339331Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir2/ mount point: /mnt/mysql/data/
[916202 20210315 18:11:50.339343Z][INFO][lib/file_handle.cc:196] create new file handle: 3 name: do_cmake.sh2
[916202 20210315 18:11:50.339352Z][INFO][lib/file.cc:29] write file meta, name: do_cmake.sh2 fh: 3 align_index: 0 crc: 4075223490
[916208 20210315 18:11:50.339317Z][INFO][lib/block_fs_fuse.cc:1523] FUSE version: 3.10.1
[916208 20210315 18:11:50.339429Z][INFO][lib/block_fs_fuse.cc:1535] fuse enable foreground
[916208 20210315 18:11:50.339444Z][INFO][lib/block_fs_fuse.cc:1575] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[916208 20210315 18:11:50.339450Z][INFO][lib/block_fs_fuse.cc:1580] argv_cnt: 6[916202 20210315 18:11:50.339453Z][INFO][lib/file_handle.cc:535] create file success: do_cmake.sh2
[916202 20210315 18:11:50.339469Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[916202 20210315 18:11:50.339472Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999

[916202 20210315 18:11:50.339474Z][INFO][lib/file_handle.cc:873] open file: /mnt/mysql/data/test_dir2/do_cmake.sh2 fd: 0 fh: 3
[916202 20210315 18:11:50.339515Z][INFO][lib/file_store_udisk.cc:632] write file fd: 0
[916202 20210315 18:11:50.339533Z][INFO][lib/file.cc:821] file name: do_cmake.sh2 file size: 0 write size: 286 offset: 0
[916202 20210315 18:11:50.339540Z][WARN][lib/file.cc:831] do_cmake.sh2 write exceed file size, file size: 0 write offset: 0 write size: 286
[916202 20210315 18:11:50.339545Z][INFO][lib/file.cc:692] file name: do_cmake.sh2 size: 0 ftruncate offset: 286
[916202 20210315 18:11:50.339550Z][INFO][lib/file.cc:316] do_cmake.sh2 need expand size: 286 offset: 286 current size: 0
[916202 20210315 18:11:50.339571Z][INFO][lib/file.cc:336] do_cmake.sh2 last block left: 0 need alloc block num: 1
[916202 20210315 18:11:50.339577Z][INFO][lib/file.cc:364] do_cmake.sh2 last file block left num: 0 left need alloc file block num: 1
[916202 20210315 18:11:50.339581Z][INFO][lib/block_handle.cc:89] current block pool size: 31962 apply block_id_num: 1
[916202 20210315 18:11:50.339586Z][INFO][lib/block_handle.cc:94] apply for a new file block id: 18628
[916202 20210315 18:11:50.339591Z][INFO][lib/file.cc:411] do_cmake.sh2 next file cut: 0
[916202 20210315 18:11:50.339596Z][INFO][lib/file.cc:417] do_cmake.sh2 fill file block, used_block_num: 0 block id: 18628
[916202 20210315 18:11:50.339600Z][INFO][lib/file.cc:421] do_cmake.sh2 fill file block, used_block_num: 1
[916202 20210315 18:11:50.339605Z][INFO][lib/file.cc:430] do_cmake.sh2 left block ids num: 0
[916202 20210315 18:11:50.339610Z][INFO][lib/file_block.cc:29] write file block meta index: 2 crc:2723590267
[916202 20210315 18:11:50.339678Z][INFO][lib/file.cc:29] write file meta, name: do_cmake.sh2 fh: 3 align_index: 0 crc: 3227368900
[916202 20210315 18:11:50.339737Z][INFO][lib/file.cc:866] current offset: 0 file_block_index: 0 block_index_in_file_block: 0 block_offset_in_block: 0
[916202 20210315 18:11:50.339745Z][INFO][lib/file.cc:279] do_cmake.sh2 get file block cut: 0 file block size: 1
[916202 20210315 18:11:50.339748Z][INFO][lib/file.cc:1026] do_cmake.sh2 data_start_offset: 603979776 need wirte offset: 0 block_write_index: 18628 block_write_offset: 0 block_write_size: 286 udisk_offset: 313129959424
[916202 20210315 18:11:50.339755Z][INFO][lib/file.cc:1052] do_cmake.sh2 write udisk offset: 313129959424 write size: 286 buffer addr: 0x94288138113024
[916202 20210315 18:11:50.339832Z][INFO][lib/file_store_udisk.cc:741] fsync file fd: 0
[916202 20210315 18:11:50.340093Z][INFO][lib/file_store_udisk.cc:457] close file fd: 0
[916202 20210315 18:11:50.340143Z][INFO][lib/file_handle.cc:897] close file name: do_cmake.sh2 fd: 0
[916202 20210315 18:11:50.350356Z][INFO][tool/block_fs_cp.cc:148] copy: /home/luotang/do_cmake.sh to /mnt/mysql/data/test_dir2/do_cmake.sh2 success
[916202 20210315 18:11:50.350380Z][INFO][tool/block_fs_cp.cc:254] copy file: /home/luotang/do_cmake.sh to /mnt/mysql/data/test_dir2/do_cmake.sh2 success
luotang@10-23-227-66:~$ 


luotang@10-23-227-66:~$ cd bfs/
luotang@10-23-227-66:~/bfs$ cd test_dir2/
luotang@10-23-227-66:~/bfs/test_dir2$ ll
total 16384
-rwxr-xr-x 0 luotang luotang 286 Mar 15 18:11 do_cmake.sh2*
luotang@10-23-227-66:~/bfs/test_dir2$ pwd
/home/luotang/bfs/test_dir2
luotang@10-23-227-66:~/bfs/test_dir2$ cat do_cmake.sh2 
#!/bin/bash
if [[ $# > 1 ]]; then
    echo "usage $0"
    exit 1
fi

if test -e build;then
    echo "-- build dir already exists; rm -rf build and re-run"
    rm -rf build
fi

mkdir build
cd build

#cmake -DCMAKE_BUILD_TYPE=Debug "$@" ..
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo "$@" ..
luotang@10-23-227-66:~/bfs/test_dir2$ 
```

```sh
BFS内部拷贝数据

luotang@10-23-227-66:~$ sudo ./blockfs/build/tool/block_fs_cp -m -s /mnt/mysql/data/block_fs_tool -t /mnt/mysql/data/test_dir2/block_fs_tool

916917 20210315 18:15:44.065377Z][INFO][lib/file.cc:821] file name: block_fs_tool file size: 8437760 write size: 6992 offset: 8437760
[916917 20210315 18:15:44.065381Z][WARN][lib/file.cc:831] block_fs_tool write exceed file size, file size: 8437760 write offset: 8437760 write size: 6992
[916917 20210315 18:15:44.065385Z][INFO][lib/file.cc:692] file name: block_fs_tool size: 8437760 ftruncate offset: 8444752
[916917 20210315 18:15:44.065389Z][INFO][lib/file.cc:316] block_fs_tool need expand size: 6992 offset: 8444752 current size: 8437760
[916917 20210315 18:15:44.065393Z][INFO][lib/file.cc:336] block_fs_tool last block left: 8339456 need alloc block num: 0
[916917 20210315 18:15:44.065397Z][INFO][lib/file.cc:279] block_fs_tool get file block cut: 0 file block size: 1
[916917 20210315 18:15:44.065401Z][INFO][lib/file.cc:364] block_fs_tool last file block left num: 999 left need alloc file block num: 0
[916917 20210315 18:15:44.065405Z][INFO][lib/file.cc:29] write file meta, name: block_fs_tool fh: 4 align_index: 0 crc: 1162127719
[916917 20210315 18:15:44.066286Z][INFO][lib/file.cc:866] current offset: 8437760 file_block_index: 0 block_index_in_file_block: 0 block_offset_in_block: 8437760
[916917 20210315 18:15:44.066291Z][INFO][lib/file.cc:279] block_fs_tool get file block cut: 0 file block size: 1
[916917 20210315 18:15:44.066296Z][INFO][lib/file.cc:1026] block_fs_tool data_start_offset: 603979776 need wirte offset: 0 block_write_index: 14483 block_write_offset: 8437760 block_write_size: 6992 udisk_offset: 243596836864
[916917 20210315 18:15:44.066302Z][INFO][lib/file.cc:1052] block_fs_tool write udisk offset: 243596836864 write size: 6992 buffer addr: 0x94548538155520
[916917 20210315 18:15:44.066361Z][INFO][lib/file_store_udisk.cc:741] fsync file fd: 1
[916917 20210315 18:15:44.142201Z][INFO][lib/file_store_udisk.cc:457] close file fd: 1
[916917 20210315 18:15:44.142254Z][INFO][lib/file_handle.cc:897] close file name: block_fs_tool fd: 1
[916917 20210315 18:15:44.142266Z][INFO][tool/block_fs_cp.cc:148] copy: /mnt/mysql/data/block_fs_tool to /mnt/mysql/data/test_dir2/block_fs_tool success
[916917 20210315 18:15:44.142272Z][INFO][tool/block_fs_cp.cc:254] copy file: /mnt/mysql/data/block_fs_tool to /mnt/mysql/data/test_dir2/block_fs_tool success

luotang@10-23-227-66:~/bfs/test_dir2$ ll
total 32768
-rwxr-xr-x 0 luotang luotang 8444752 Mar 15 18:15 block_fs_tool*
-rwxr-xr-x 0 luotang luotang     286 Mar 15 18:11 do_cmake.sh2*
luotang@10-23-227-66:~/bfs/test_dir2$ ./block_fs_tool 
[917150 20210315 18:16:34.481156Z][ERROR][tool/block_fs_tool.cc:209] no option exist, Please check
[917150 20210315 18:16:34.481236Z][INFO][tool/block_fs_tool.cc:217] Build time    : Mar 12 2021 11:40:25
[917150 20210315 18:16:34.481243Z][INFO][tool/block_fs_tool.cc:218] Build version : 0x1
[917150 20210315 18:16:34.481251Z][INFO][tool/block_fs_tool.cc:219] Run options:
[917150 20210315 18:16:34.481254Z][INFO][tool/block_fs_tool.cc:220]  -d, --device   Device such as: /dev/vdb
[917150 20210315 18:16:34.481256Z][INFO][tool/block_fs_tool.cc:221]  -f, --format   Format device.
[917150 20210315 18:16:34.481261Z][INFO][tool/block_fs_tool.cc:222]  -p, --dump     Dump device, xxx args.
[917150 20210315 18:16:34.481263Z][INFO][tool/block_fs_tool.cc:223]  -s, --simu     Mount point path.
[917150 20210315 18:16:34.481269Z][INFO][tool/block_fs_tool.cc:224]  -c, --check    Check format whether success.
[917150 20210315 18:16:34.481271Z][INFO][tool/block_fs_tool.cc:225]  -m, --meta     Dump file meta content.
[917150 20210315 18:16:34.481273Z][INFO][tool/block_fs_tool.cc:226]  -b, --blk      Export blk device content.
[917150 20210315 18:16:34.481276Z][INFO][tool/block_fs_tool.cc:227]  -v, --version  Print the version.
[917150 20210315 18:16:34.481280Z][INFO][tool/block_fs_tool.cc:228]  -h, --help     Print help info.
[917150 20210315 18:16:34.481283Z][INFO][tool/block_fs_tool.cc:230] Examples:
[917150 20210315 18:16:34.481287Z][INFO][tool/block_fs_tool.cc:231]  help    : ./block_fs_tool -h
[917150 20210315 18:16:34.481290Z][INFO][tool/block_fs_tool.cc:232]  version : ./block_fs_tool -v
[917150 20210315 18:16:34.481292Z][INFO][tool/block_fs_tool.cc:233]  format  : ./block_fs_tool -d /dev/vdb -f
[917150 20210315 18:16:34.481294Z][INFO][tool/block_fs_tool.cc:234]  format  : ./block_fs_tool -d /dev/vdb --format
[917150 20210315 18:16:34.481298Z][INFO][tool/block_fs_tool.cc:235]  dump    : ./block_fs_tool -d /dev/vdb -p check
[917150 20210315 18:16:34.481301Z][INFO][tool/block_fs_tool.cc:236]  dump    : ./block_fs_tool -d /dev/vdb -p negot
[917150 20210315 18:16:34.481305Z][INFO][tool/block_fs_tool.cc:237]  dump    : ./block_fs_tool -d /dev/vdb -p super
[917150 20210315 18:16:34.481309Z][INFO][tool/block_fs_tool.cc:238]  dump    : ./block_fs_tool -d /dev/vdb -p uuid
[917150 20210315 18:16:34.481311Z][INFO][tool/block_fs_tool.cc:239]  dump    : ./block_fs_tool -d /dev/vdb -p dirs
[917150 20210315 18:16:34.481316Z][INFO][tool/block_fs_tool.cc:240]  dump    : ./block_fs_tool -d /dev/vdb -p files
[917150 20210315 18:16:34.481321Z][INFO][tool/block_fs_tool.cc:241]  dump    : ./block_fs_tool -d /dev/vdb -p all
[917150 20210315 18:16:34.481324Z][INFO][tool/block_fs_tool.cc:242]  meta    : ./block_fs_tool -d /dev/vdb -m file_name
[917150 20210315 18:16:34.481326Z][INFO][tool/block_fs_tool.cc:243]  dump    : ./block_fs_tool -d /dev/vdb --dump xxx
[917150 20210315 18:16:34.481330Z][INFO][tool/block_fs_tool.cc:244]  blk     : ./block_fs_tool -d /dev/vdb -b offset:size
[917150 20210315 18:16:34.481334Z][INFO][tool/block_fs_tool.cc:245]  cat     : ./block_fs_tool -d /dev/vdb -c /mnt/mysql/data/1.ibt
[917150 20210315 18:16:34.481338Z][INFO][tool/block_fs_tool.cc:247]  simu    : ./block_fs_tool -d /dev/vdb -s /mnt/mysql/data/
[917150 20210315 18:16:34.481342Z][INFO][tool/block_fs_tool.cc:248]  simu    : [ls,pwd,cd,mkdir,stat,cat,echo,rename,rmdir,touch,rm,unlink]
luotang@10-23-227-66:~/bfs/test_dir2$ pwd
/home/luotang/bfs/test_dir2
luotang@10-23-227-66:~/bfs/test_dir2$ 
```

### block_fs_cp_range
```sh

luotang@10-23-227-66:~$ sudo ./blockfs/build/tool/block_fs_cp_range -m -s /mnt/mysql/data/do_cmake.sh -t /home/luotang/do_cmake.sh.bak -o 10 -l 100
[941472 20210315 20:19:09.368254Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[941472 20210315 20:19:09.368351Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir2/ seq_no: 0 crc: 2171320134
[941472 20210315 20:19:09.462183Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[941472 20210315 20:19:09.462206Z][INFO][lib/super_block.cc:173] mount point has been configured
[941472 20210315 20:19:09.462208Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[941472 20210315 20:19:09.462277Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh mount point: /mnt/mysql/data/
[941472 20210315 20:19:09.462284Z][INFO][lib/super_block.cc:235] check target path: /home/luotang/do_cmake.sh.bak mount point: /mnt/mysql/data/
[941472 20210315 20:19:09.462323Z][ERROR][lib/super_block.cc:246] path not startwith mount prefix: /home/luotang/do_cmake.sh.bak
[941472 20210315 20:19:09.462326Z][INFO][lib/file_store_udisk.cc:447] open file: /mnt/mysql/data/do_cmake.sh
[941472 20210315 20:19:09.462338Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh/ mount point: /mnt/mysql/data/
[941472 20210315 20:19:09.462348Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/data/do_cmake.sh/
[941472 20210315 20:19:09.462351Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/do_cmake.sh
[941472 20210315 20:19:09.462357Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/do_cmake.sh mount point: /mnt/mysql/data/
[941472 20210315 20:19:09.462372Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[941472 20210315 20:19:09.462377Z][INFO][lib/file_handle.cc:141] transformPath filename: do_cmake.sh
[941472 20210315 20:19:09.462383Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[941472 20210315 20:19:09.462387Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[941472 20210315 20:19:09.462396Z][INFO][lib/file_handle.cc:873] open file: /mnt/mysql/data/do_cmake.sh fd: 0 fh: 1
[941472 20210315 20:19:09.462405Z][ERROR][tool/block_fs_cp_range.cc:68] read file range size: 100
[941472 20210315 20:19:09.462411Z][INFO][lib/file_store_udisk.cc:674] lseek file fd: 0 offset: 10
[941472 20210315 20:19:09.462416Z][WARN][lib/file.cc:763] do_cmake.sh lseek fh: 1 offset: 0
[941475 20210315 20:19:09.462349Z][INFO][lib/block_fs_fuse.cc:1523] FUSE version: 3.10.1
[941475 20210315 20:19:09.462453Z][INFO][lib/block_fs_fuse.cc:1535] fuse enable foreground
[941475 20210315 20:19:09.462467Z][INFO][lib/block_fs_fuse.cc:1575] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[941475 20210315 20:19:09.462475Z][INFO][lib/block_fs_fuse.cc:1580] argv_cnt: 6
[941472 20210315 20:19:09.462476Z][INFO][lib/file_store_udisk.cc:619] read file fd: 0 len: 100
[941472 20210315 20:19:09.462528Z][INFO][lib/file.cc:791] file name: do_cmake.sh file size: 286 read size: 100 offset: 10
[941472 20210315 20:19:09.462538Z][INFO][lib/file.cc:866] current offset: 10 file_block_index: 0 block_index_in_file_block: 0 block_offset_in_block: 10
[941472 20210315 20:19:09.462549Z][INFO][lib/file.cc:279] do_cmake.sh get file block cut: 0 file block size: 1
[941472 20210315 20:19:09.462559Z][INFO][lib/file.cc:919] do_cmake.sh data_start_offset: 603979776 need read offset: 0 read block index: 1467 read block offset: 10 block_read_size: 100 udisk_offset: 25216155658
[941472 20210315 20:19:09.462569Z][INFO][lib/file.cc:946] do_cmake.sh read udisk offset: 25216155658 read size: 100 buffer addr: 0x94912275715072
[941472 20210315 20:19:09.463685Z][INFO][tool/block_fs_cp_range.cc:167] copy file range: /mnt/mysql/data/do_cmake.sh to /home/luotang/do_cmake.sh.bak success
[941472 20210315 20:19:09.463733Z][INFO][tool/block_fs_cp_range.cc:259] copy file: /mnt/mysql/data/do_cmake.sh to /home/luotang/do_cmake.sh.bak success
luotang@10-23-227-66:~$ cat do_cmake.sh
#!/bin/bash
if [[ $# > 1 ]]; then
    echo "usage $0"
    exit 1
fi

if test -e build;then
    echo "-- build dir already exists; rm -rf build and re-run"
    rm -rf build
fi

mkdir build
cd build

#cmake -DCMAKE_BUILD_TYPE=Debug "$@" ..
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo "$@" ..
luotang@10-23-227-66:~$ cat do_cmake.sh.bak 
h
if [[ $# > 1 ]]; then
    echo "usage $0"
    exit 1
fi

if test -e build;then
    echo "-- build luotang@10-23-227-66:~$ 

```



### 7. block_fs_mkdir工具

```
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_mkdir -m -n /mnt/mysql/data/sbtest
[858370 20210315 14:04:11.628064Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[858370 20210315 14:04:11.737244Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[858370 20210315 14:04:11.737268Z][INFO][lib/super_block.cc:173] mount point has been configured
[858370 20210315 14:04:11.737272Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[858370 20210315 14:04:11.737362Z][INFO][lib/file_store_udisk.cc:175] make directory: /mnt/mysql/data/sbtest
[858370 20210315 14:04:11.737374Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/sbtest
[858370 20210315 14:04:11.737380Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest mount point: /mnt/mysql/data/
[858370 20210315 14:04:11.737390Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[858370 20210315 14:04:11.737393Z][INFO][lib/file_handle.cc:141] transformPath filename: sbtest
[858370 20210315 14:04:11.737403Z][WARN][lib/file_handle.cc:725] file not exist: sbtest
[858370 20210315 14:04:11.737411Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest/ mount point: /mnt/mysql/data/
[858370 20210315 14:04:11.737421Z][INFO][lib/dir_handle.cc:218] create new directory handle: 1
[858370 20210315 14:04:11.737429Z][INFO][lib/directory.cc:147] directory handle: 1 align_index: 0
[858376 20210315 14:04:11.737439Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[858376 20210315 14:04:11.737511Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[858376 20210315 14:04:11.737523Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[858376 20210315 14:04:11.737529Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[858370 20210315 14:04:11.737529Z][INFO][lib/dir_handle.cc:465] make directory success: /mnt/mysql/data/sbtest
[858370 20210315 14:04:11.737536Z][INFO][tool/block_fs_mkdir.cc:72] mkdir dir: /mnt/mysql/data/sbtest success
luotang@10-23-227-66:~/blockfs$ 
```


### 8. block_fs_rmdir工具
```sh
luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_rmdir -m -n /mnt/mysql/data/sbtest
[862075 20210315 14:13:34.913148Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[862075 20210315 14:13:34.913251Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/sbtest/ seq_no: 0 crc: 96374263
[862075 20210315 14:13:35.027737Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[862075 20210315 14:13:35.027773Z][INFO][lib/super_block.cc:173] mount point has been configured
[862075 20210315 14:13:35.027776Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[862075 20210315 14:13:35.027853Z][INFO][lib/file_store_udisk.cc:205] remove directory: /mnt/mysql/data/sbtest
[862075 20210315 14:13:35.027863Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/sbtest
[862075 20210315 14:13:35.027881Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest mount point: /mnt/mysql/data/
[862075 20210315 14:13:35.027894Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[862075 20210315 14:13:35.027897Z][INFO][lib/file_handle.cc:141] transformPath filename: sbtest
[862075 20210315 14:13:35.027915Z][WARN][lib/file_handle.cc:725] file not exist: sbtest
[862082 20210315 14:13:35.027878Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[862082 20210315 14:13:35.027944Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[862075 20210315 14:13:35.027950Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/sbtest/ mount point: /mnt/mysql/data/
[862082 20210315 14:13:35.027954Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[862082 20210315 14:13:35.027965Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
[862075 20210315 14:13:35.027975Z][INFO][lib/directory.cc:147] directory handle: 1 align_index: 0
[862075 20210315 14:13:35.028141Z][INFO][lib/dir_handle.cc:540] remove directory success: /mnt/mysql/data/sbtest
[862075 20210315 14:13:35.028150Z][INFO][tool/block_fs_rmdir.cc:72] rm dir: /mnt/mysql/data/sbtest success
luotang@10-23-227-66:~/blockfs$ 
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
luotang@10-23-227-66:~/blockfs$ 
```


#### block_fs_rm工具
```sh
sudo ./build/tool/block_fs_rm -n /mnt/mysql/data/test_dir -m

luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_rm -n /mnt/mysql/data/test_dir -m
[874517 20210315 15:28:00.269320Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/data/ seq_no: 0 crc: 802599935
[874517 20210315 15:28:00.269436Z][WARN][lib/dir_handle.cc:39] directory handle: 1 name: /mnt/mysql/data/test_dir/ seq_no: 0 crc: 1653569268
[874517 20210315 15:28:00.362903Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/data/
[874517 20210315 15:28:00.362924Z][INFO][lib/super_block.cc:173] mount point has been configured
[874517 20210315 15:28:00.362927Z][INFO][lib/file_store_udisk.cc:1053] create fs mount point: /mnt/mysql/data/
[874517 20210315 15:28:00.363006Z][INFO][lib/file_store_udisk.cc:779] remove path: /mnt/mysql/data/test_dir
[874517 20210315 15:28:00.363017Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_dir
[874517 20210315 15:28:00.363021Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir mount point: /mnt/mysql/data/
[874517 20210315 15:28:00.363028Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[874517 20210315 15:28:00.363033Z][INFO][lib/file_handle.cc:141] transformPath filename: test_dir
[874517 20210315 15:28:00.363101Z][WARN][lib/file_handle.cc:725] file not exist: test_dir
[874517 20210315 15:28:00.363119Z][INFO][lib/file_handle.cc:705] get created file name: /mnt/mysql/data/test_dir
[874517 20210315 15:28:00.363154Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir mount point: /mnt/mysql/data/
[874517 20210315 15:28:00.363162Z][INFO][lib/file_handle.cc:140] transformPath dirname: /mnt/mysql/data/
[874517 20210315 15:28:00.363165Z][INFO][lib/file_handle.cc:141] transformPath filename: test_dir
[874517 20210315 15:28:00.363170Z][WARN][lib/file_handle.cc:725] file not exist: test_dir
[874517 20210315 15:28:00.363173Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/data/test_dir/ mount point: /mnt/mysql/data/
[874517 20210315 15:28:00.363179Z][INFO][lib/directory.cc:147] directory handle: 1 align_index: 0
[874521 20210315 15:28:00.363092Z][INFO][lib/block_fs_fuse.cc:1522] FUSE version: 3.10.1
[874521 20210315 15:28:00.363209Z][INFO][lib/block_fs_fuse.cc:1534] fuse enable foreground
[874521 20210315 15:28:00.363221Z][INFO][lib/block_fs_fuse.cc:1574] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[874521 20210315 15:28:00.363229Z][INFO][lib/block_fs_fuse.cc:1579] argv_cnt: 6
fuse: bad mount point `/home/luotang/bfs/': Transport endpoint is not connected
[874517 20210315 15:28:00.363342Z][INFO][lib/dir_handle.cc:540] remove directory success: /mnt/mysql/data/test_dir
[874517 20210315 15:28:00.363352Z][INFO][tool/block_fs_rm.cc:72] rm file: /mnt/mysql/data/test_dir success
luotang@10-23-227-66:~/blockfs$ 
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
luotang@10-23-227-66:~/blockfs$ 


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


### block_fs_stub.py

##### ```测试立即kill进程```

```sh

luotang@10-23-227-66:~/blockfs$ sudo python3 tool/block_fs_stub.py 
[sudo] password for luotang: 
send requuest to server '/data/mysql/bfs.sock': kill_now
recv responce from server '/data/mysql/bfs.sock': 
luotang@10-23-227-66:~/blockfs$ 


luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_mount -c conf/bfs.cnf -m
[sudo] password for luotang: 
[279676 20210525 15:59:04.857420Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/ seq_no: 0 crc: 3261875897
[279676 20210525 15:59:04.957272Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/
[279676 20210525 15:59:04.957309Z][INFO][lib/super_block.cc:173] mount point has been configured
[279676 20210525 15:59:04.957312Z][INFO][lib/file_store_udisk.cc:1051] create fs mount point: /mnt/mysql/
[279676 20210525 15:59:04.957416Z][ERROR][lib/injection.cc:146] chmod access failed for /data/mysql/bfs.sock
[279676 20210525 15:59:04.957584Z][INFO][lib/block_fs_fuse.cc:1632] FUSE version: 3.10.1
[279676 20210525 15:59:04.957611Z][INFO][lib/block_fs_fuse.cc:1644] fuse enable foreground
[279676 20210525 15:59:04.957619Z][INFO][lib/block_fs_fuse.cc:1684] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[279676 20210525 15:59:04.957625Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/
[279676 20210525 15:59:04.957666Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/mysql/
[279676 20210525 15:59:04.957672Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/mysql/bfs/
[279676 20210525 15:59:04.957682Z][INFO][lib/block_fs_fuse.cc:1696] argv_cnt: 6
[279688 20210525 15:59:04.964043Z][INFO][lib/block_fs_fuse.cc:1013] call bfs_init
[279688 20210525 15:59:04.964963Z][INFO][lib/block_fs_fuse.cc:955] Protocol version: 7.27
[279688 20210525 15:59:04.964967Z][INFO][lib/block_fs_fuse.cc:957] Capabilities: 
[279688 20210525 15:59:04.964969Z][INFO][lib/block_fs_fuse.cc:959] FUSE_CAP_WRITEBACK_CACHE
[279688 20210525 15:59:04.964971Z][INFO][lib/block_fs_fuse.cc:960] FUSE_CAP_ASYNC_READ
[279688 20210525 15:59:04.964973Z][INFO][lib/block_fs_fuse.cc:961] FUSE_CAP_POSIX_LOCKS
[279688 20210525 15:59:04.964975Z][INFO][lib/block_fs_fuse.cc:963] FUSE_CAP_ATOMIC_O_TRUNC
[279688 20210525 15:59:04.964978Z][INFO][lib/block_fs_fuse.cc:965] FUSE_CAP_EXPORT_SUPPORT
[279688 20210525 15:59:04.964980Z][INFO][lib/block_fs_fuse.cc:966] FUSE_CAP_DONT_MASK
[279688 20210525 15:59:04.964982Z][INFO][lib/block_fs_fuse.cc:967] FUSE_CAP_SPLICE_MOVE
[279688 20210525 15:59:04.964984Z][INFO][lib/block_fs_fuse.cc:968] FUSE_CAP_SPLICE_READ
[279688 20210525 15:59:04.964986Z][INFO][lib/block_fs_fuse.cc:970] FUSE_CAP_SPLICE_WRITE
[279688 20210525 15:59:04.964988Z][INFO][lib/block_fs_fuse.cc:971] FUSE_CAP_FLOCK_LOCKS
[279688 20210525 15:59:04.964990Z][INFO][lib/block_fs_fuse.cc:972] FUSE_CAP_IOCTL_DIR
[279688 20210525 15:59:04.964992Z][INFO][lib/block_fs_fuse.cc:974] FUSE_CAP_AUTO_INVAL_DATA
[279688 20210525 15:59:04.964994Z][INFO][lib/block_fs_fuse.cc:975] FUSE_CAP_READDIRPLUS
[279688 20210525 15:59:04.964996Z][INFO][lib/block_fs_fuse.cc:977] FUSE_CAP_READDIRPLUS_AUTO
[279688 20210525 15:59:04.964999Z][INFO][lib/block_fs_fuse.cc:978] FUSE_CAP_ASYNC_DIO
[279688 20210525 15:59:04.965001Z][INFO][lib/block_fs_fuse.cc:980] FUSE_CAP_WRITEBACK_CACHE
[279688 20210525 15:59:04.965003Z][INFO][lib/block_fs_fuse.cc:982] FUSE_CAP_NO_OPEN_SUPPORT
[279688 20210525 15:59:04.965005Z][INFO][lib/block_fs_fuse.cc:984] FUSE_CAP_PARALLEL_DIROPS
[279688 20210525 15:59:04.965007Z][INFO][lib/block_fs_fuse.cc:985] FUSE_CAP_POSIX_ACL
[279686 20210525 15:59:23.312595Z][INFO][lib/injection.cc:193] new conn from unix path: /data/mysql/bfs.sock
[279686 20210525 15:59:23.312657Z][INFO][lib/injection.cc:217] new conn from unix path: 
[279686 20210525 15:59:23.312701Z][WARN][lib/injection.cc:50] stub kill right now
Killed
luotang@10-23-227-66:~/blockfs$ 
```

##### ```测试桩点kill进程```

```sh
if __name__ == "__main__":
    socket_client_obj = SocketClient()
    socket_client_obj.connect_to_server("file_open")
    # socket_client_obj.connect_to_server("kill_now")
    
luotang@10-23-227-66:~/blockfs$ sudo python3 tool/block_fs_stub.py 
send requuest to server '/data/mysql/bfs.sock': file_open
recv responce from server '/data/mysql/bfs.sock': success
luotang@10-23-227-66:~/blockfs$ 


可以看到在创建文件的时候, 触发了故障

luotang@10-23-227-66:/data/mysql$ cd bfs/
luotang@10-23-227-66:/data/mysql/bfs$ 
luotang@10-23-227-66:/data/mysql/bfs$ 
luotang@10-23-227-66:/data/mysql/bfs$ touch 2^C
luotang@10-23-227-66:/data/mysql/bfs$ ll
total 16389
drwxr-xr-x 2 luotang luotang    1024 May 25 16:09 ./
drwxr-xr-x 3 root    root       4096 May 25 16:08 ../
-rwxr-xr-x 0 luotang luotang 1600000 May 25 14:12 1.txt*
luotang@10-23-227-66:/data/mysql/bfs$ touch 2.log
touch: cannot touch '2.log': Software caused connection abort
luotang@10-23-227-66:/data/mysql/bfs$ 

luotang@10-23-227-66:~/blockfs$ sudo ./build/tool/block_fs_mount -c conf/bfs.cnf -m
[283296 20210525 16:08:34.962504Z][WARN][lib/dir_handle.cc:39] directory handle: 0 name: /mnt/mysql/ seq_no: 0 crc: 3261875897
[283296 20210525 16:08:35.058574Z][INFO][lib/super_block.cc:157] UXDB root path: /mnt/mysql/
[283296 20210525 16:08:35.058609Z][INFO][lib/super_block.cc:173] mount point has been configured
[283296 20210525 16:08:35.058611Z][INFO][lib/file_store_udisk.cc:1051] create fs mount point: /mnt/mysql/
[283296 20210525 16:08:35.058685Z][ERROR][lib/injection.cc:146] chmod access failed for /data/mysql/bfs.sock
[283296 20210525 16:08:35.058767Z][INFO][lib/block_fs_fuse.cc:1632] FUSE version: 3.10.1
[283296 20210525 16:08:35.058782Z][INFO][lib/block_fs_fuse.cc:1644] fuse enable foreground
[283296 20210525 16:08:35.058789Z][INFO][lib/block_fs_fuse.cc:1684] fuse mount_arg: allow_other,uid=1001,gid=1001,auto_unmount,entry_timeout=0.500000,attr_timeout=0.500000
[283296 20210525 16:08:35.058794Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/
[283296 20210525 16:08:35.058803Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/mysql/
[283296 20210525 16:08:35.058808Z][INFO][lib/block_fs_fuse.cc:1609] create dir: /data/mysql/bfs/
[283296 20210525 16:08:35.058818Z][INFO][lib/block_fs_fuse.cc:1696] argv_cnt: 6
[283313 20210525 16:08:35.064280Z][INFO][lib/block_fs_fuse.cc:1013] call bfs_init
[283313 20210525 16:08:35.064360Z][INFO][lib/block_fs_fuse.cc:955] Protocol version: 7.27
[283313 20210525 16:08:35.064365Z][INFO][lib/block_fs_fuse.cc:957] Capabilities: 
[283313 20210525 16:08:35.064369Z][INFO][lib/block_fs_fuse.cc:959] FUSE_CAP_WRITEBACK_CACHE
[283313 20210525 16:08:35.064373Z][INFO][lib/block_fs_fuse.cc:960] FUSE_CAP_ASYNC_READ
[283313 20210525 16:08:35.064377Z][INFO][lib/block_fs_fuse.cc:961] FUSE_CAP_POSIX_LOCKS
[283313 20210525 16:08:35.064380Z][INFO][lib/block_fs_fuse.cc:963] FUSE_CAP_ATOMIC_O_TRUNC
[283313 20210525 16:08:35.064384Z][INFO][lib/block_fs_fuse.cc:965] FUSE_CAP_EXPORT_SUPPORT
[283313 20210525 16:08:35.064388Z][INFO][lib/block_fs_fuse.cc:966] FUSE_CAP_DONT_MASK
[283313 20210525 16:08:35.064392Z][INFO][lib/block_fs_fuse.cc:967] FUSE_CAP_SPLICE_MOVE
[283313 20210525 16:08:35.064395Z][INFO][lib/block_fs_fuse.cc:968] FUSE_CAP_SPLICE_READ
[283313 20210525 16:08:35.064397Z][INFO][lib/block_fs_fuse.cc:970] FUSE_CAP_SPLICE_WRITE
[283313 20210525 16:08:35.064399Z][INFO][lib/block_fs_fuse.cc:971] FUSE_CAP_FLOCK_LOCKS
[283313 20210525 16:08:35.064403Z][INFO][lib/block_fs_fuse.cc:972] FUSE_CAP_IOCTL_DIR
[283313 20210525 16:08:35.064407Z][INFO][lib/block_fs_fuse.cc:974] FUSE_CAP_AUTO_INVAL_DATA
[283313 20210525 16:08:35.064411Z][INFO][lib/block_fs_fuse.cc:975] FUSE_CAP_READDIRPLUS
[283313 20210525 16:08:35.064414Z][INFO][lib/block_fs_fuse.cc:977] FUSE_CAP_READDIRPLUS_AUTO
[283313 20210525 16:08:35.064418Z][INFO][lib/block_fs_fuse.cc:978] FUSE_CAP_ASYNC_DIO
[283313 20210525 16:08:35.064422Z][INFO][lib/block_fs_fuse.cc:980] FUSE_CAP_WRITEBACK_CACHE
[283313 20210525 16:08:35.064425Z][INFO][lib/block_fs_fuse.cc:982] FUSE_CAP_NO_OPEN_SUPPORT
[283313 20210525 16:08:35.064427Z][INFO][lib/block_fs_fuse.cc:984] FUSE_CAP_PARALLEL_DIROPS
[283313 20210525 16:08:35.064432Z][INFO][lib/block_fs_fuse.cc:985] FUSE_CAP_POSIX_ACL
[283311 20210525 16:08:40.000853Z][INFO][lib/injection.cc:193] new conn from unix path: /data/mysql/bfs.sock
[283311 20210525 16:08:40.000911Z][INFO][lib/injection.cc:217] new conn from unix path: 
[283311 20210525 16:08:40.000930Z][WARN][lib/injection.cc:54] inser key: file_open count: 1
[283311 20210525 16:08:40.000982Z][WARN][lib/injection.cc:256] delete connection: 9
[283313 20210525 16:08:52.379446Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /
[283314 20210525 16:08:53.495469Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /
[283313 20210525 16:08:53.660435Z][INFO][lib/block_fs_fuse.cc:1060] call bfs_access: /
[283313 20210525 16:08:53.660504Z][INFO][lib/file_store_udisk.cc:282] access path name: /mnt/mysql/
[283313 20210525 16:08:53.660533Z][INFO][lib/file_handle.cc:707] get created file name: /mnt/mysql/
[283313 20210525 16:08:53.660580Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/ mount point: /mnt/mysql/
[283313 20210525 16:08:53.660586Z][ERROR][lib/super_block.cc:262] file cannot endwith dir separator: /mnt/mysql/
[283314 20210525 16:09:02.776339Z][INFO][lib/block_fs_fuse.cc:836] call bfs_opendir: /
[283314 20210525 16:09:02.776452Z][INFO][lib/file_store_udisk.cc:227] open directory: /mnt/mysql/
[283314 20210525 16:09:02.776498Z][INFO][lib/dir_handle.cc:604] open directory /mnt/mysql/
[283314 20210525 16:09:02.776547Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[283314 20210525 16:09:02.776582Z][INFO][lib/fd_handle.cc:37] current fd: 0 pool size: 399999
[283313 20210525 16:09:02.776816Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /
[283314 20210525 16:09:02.776947Z][INFO][lib/block_fs_fuse.cc:876] call bfs_readdir: /
[283314 20210525 16:09:02.776960Z][INFO][lib/block_fs_fuse.cc:885] readdir: /mnt/mysql/
[283314 20210525 16:09:02.776976Z][INFO][lib/directory.cc:107] init scan directory: /mnt/mysql/
[283314 20210525 16:09:02.776989Z][INFO][lib/directory.cc:127] scan file: 1.txt
[283313 20210525 16:09:02.777048Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /
[283314 20210525 16:09:02.777117Z][INFO][lib/block_fs_fuse.cc:810] call bfs_getxattr file: /
[283313 20210525 16:09:02.777157Z][INFO][lib/block_fs_fuse.cc:810] call bfs_getxattr file: /
[283314 20210525 16:09:02.777537Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /1.txt
[283314 20210525 16:09:02.777551Z][INFO][lib/file_store_udisk.cc:317] stat path: /mnt/mysql/1.txt
[283314 20210525 16:09:02.777559Z][INFO][lib/file_store_udisk.cc:326] stat path: /mnt/mysql/1.txt
[283314 20210525 16:09:02.777568Z][INFO][lib/file_handle.cc:707] get created file name: /mnt/mysql/1.txt
[283314 20210525 16:09:02.777588Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/1.txt mount point: /mnt/mysql/
[283314 20210525 16:09:02.777605Z][INFO][lib/file_handle.cc:142] transformPath dirname: /mnt/mysql/
[283314 20210525 16:09:02.777617Z][INFO][lib/file_handle.cc:143] transformPath filename: 1.txt
[283313 20210525 16:09:02.777680Z][INFO][lib/block_fs_fuse.cc:810] call bfs_getxattr file: /1.txt
[283313 20210525 16:09:02.777749Z][INFO][lib/block_fs_fuse.cc:940] call bfs_releasedir: /
[283313 20210525 16:09:02.777757Z][INFO][lib/dir_handle.cc:654] close directory name: /mnt/mysql/ fd: 0
[283314 20210525 16:09:07.948751Z][INFO][lib/block_fs_fuse.cc:146] call bfs_getattr file: /2.log
[283314 20210525 16:09:07.948861Z][INFO][lib/file_store_udisk.cc:317] stat path: /mnt/mysql/2.log
[283314 20210525 16:09:07.948904Z][INFO][lib/file_store_udisk.cc:326] stat path: /mnt/mysql/2.log
[283314 20210525 16:09:07.949006Z][INFO][lib/file_handle.cc:707] get created file name: /mnt/mysql/2.log
[283314 20210525 16:09:07.949016Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/2.log mount point: /mnt/mysql/
[283314 20210525 16:09:07.949042Z][INFO][lib/file_handle.cc:142] transformPath dirname: /mnt/mysql/
[283314 20210525 16:09:07.949049Z][INFO][lib/file_handle.cc:143] transformPath filename: 2.log
[283314 20210525 16:09:07.949060Z][WARN][lib/file_handle.cc:727] file not exist: 2.log
[283314 20210525 16:09:07.949070Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/2.log/ mount point: /mnt/mysql/
[283314 20210525 16:09:07.949090Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/2.log/
[283313 20210525 16:09:07.949240Z][INFO][lib/block_fs_fuse.cc:1084] call bfs_create: /2.log
[283313 20210525 16:09:07.949289Z][INFO][lib/file_store_udisk.cc:445] open file: /mnt/mysql/2.log
[283313 20210525 16:09:07.949313Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/2.log/ mount point: /mnt/mysql/
[283313 20210525 16:09:07.949329Z][WARN][lib/dir_handle.cc:410] directory not exist: /mnt/mysql/2.log/
[283313 20210525 16:09:07.949336Z][INFO][lib/file_handle.cc:707] get created file name: /mnt/mysql/2.log
[283313 20210525 16:09:07.949352Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/2.log mount point: /mnt/mysql/
[283313 20210525 16:09:07.949369Z][INFO][lib/file_handle.cc:142] transformPath dirname: /mnt/mysql/
[283313 20210525 16:09:07.949379Z][INFO][lib/file_handle.cc:143] transformPath filename: 2.log
[283313 20210525 16:09:07.949385Z][WARN][lib/file_handle.cc:727] file not exist: 2.log
[283313 20210525 16:09:07.949390Z][INFO][lib/file_handle.cc:493] create file: /mnt/mysql/2.log
[283313 20210525 16:09:07.949394Z][INFO][lib/super_block.cc:235] check target path: /mnt/mysql/2.log mount point: /mnt/mysql/
[283313 20210525 16:09:07.949398Z][INFO][lib/file_handle.cc:142] transformPath dirname: /mnt/mysql/
[283313 20210525 16:09:07.949402Z][INFO][lib/file_handle.cc:143] transformPath filename: 2.log
[283313 20210525 16:09:07.949416Z][INFO][lib/file_handle.cc:198] create new file handle: 1 name: 2.log
[283313 20210525 16:09:07.949431Z][INFO][lib/file.cc:29] write file meta, name: 2.log fh: 1 align_index: 0 crc: 1572939721
[283313 20210525 16:09:07.949598Z][INFO][lib/file_handle.cc:537] create file success: 2.log
[283313 20210525 16:09:07.949624Z][INFO][lib/fd_handle.cc:34] current fd pool size: 400000
[283313 20210525 16:09:07.949656Z][INFO][lib/fd_handle.cc:37] current fd: 1 pool size: 399999
[283313 20210525 16:09:07.949676Z][INFO][lib/file_handle.cc:875] open file: /mnt/mysql/2.log fd: 1 fh: 1
[283313 20210525 16:09:07.949690Z][WARN][lib/injection.cc:63] stub kill at key: file_open
Killed
luotang@10-23-227-66:~/blockfs$ 

```