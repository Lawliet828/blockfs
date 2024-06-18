配置：

4核8G
40G RSSD数据盘 QOS iops:100000 bw:800MB/s

write-iops
fio --name=4k_rw --ioengine=libaio --blocksize=4k --readwrite=randwrite --filesize=64M --numjobs=4 --iodepth=32 --direct=1 --group_reporting -time_based=1 -runtime=90

fio --ioengine=libaio --name=4k_rw --direct=1 --iodepth=32 --rw=write --blocksize=4k --numjobs=4 --size=64M --fallocate=none --verify=md5 --do_verify=1 -time_based=1 -runtime=90


4K随机读写
```
fio -direct=1 -iodepth=32 -rw=randrw -ioengine=libaio -bs=4k -size=512M -numjobs=1 -group_reporting -filename=/data/mysql/bfs/testfile -fallocate=none -name=4K_RandRW -verify=md5 --do_verify=1 --verify_fatal=1
```

2023/03/07 11:40 rssd云盘
   READ: bw=200KiB/s (204kB/s), 200KiB/s-200KiB/s (204kB/s-204kB/s), io=58.5MiB (61.3MB), run=299990-299990msec
  WRITE: bw=156KiB/s (159kB/s), 156KiB/s-156KiB/s (159kB/s-159kB/s), io=32.1MiB (33.7MB), run=211460-211460msec

2023/03/08 00:05 ssd本地盘
   READ: bw=75.7KiB/s (77.5kB/s), 75.7KiB/s-75.7KiB/s (77.5kB/s-77.5kB/s), io=22.2MiB (23.3MB), run=300348-300348msec
  WRITE: bw=76.0KiB/s (77.9kB/s), 76.0KiB/s-76.0KiB/s (77.9kB/s-77.9kB/s), io=22.3MiB (23.4MB), run=300348-300348msec

2024/05/31 21:30 rssd云盘 去掉file锁, 使用block锁
   READ: bw=264KiB/s (270kB/s), 264KiB/s-264KiB/s (270kB/s-270kB/s), io=64.0MiB (67.1MB), run=248181-248181msec
  WRITE: bw=199KiB/s (204kB/s), 199KiB/s-199KiB/s (204kB/s-204kB/s), io=32.1MiB (33.7MB), run=165081-165081msec

2024/06/14 00:55 rssd云盘 读写流程使用spdlog
   READ: bw=226MiB/s (237MB/s), 226MiB/s-226MiB/s (237MB/s-237MB/s), io=64.0MiB (67.1MB), run=283-283msec
  WRITE: bw=188MiB/s (197MB/s), 188MiB/s-188MiB/s (197MB/s-197MB/s), io=32.1MiB (33.7MB), run=171-171msec

```
fio -direct=1 -iodepth=32 -rw=randrw -ioengine=libaio -bsrange=1k-16k -size=1G -numjobs=8 -group_reporting -fallocate=none -name=4KRandRW -verify=md5 --do_verify=1 --verify_fatal=1
```

2024/06/14 16:15 rssd云盘 使用spdlog
   READ: bw=187MiB/s (196MB/s), 187MiB/s-187MiB/s (196MB/s-196MB/s), io=8192MiB (8590MB), run=43733-43733msec
  WRITE: bw=139MiB/s (146MB/s), 139MiB/s-139MiB/s (146MB/s-146MB/s), io=4096MiB (4295MB), run=29508-29508msec

## mdtest

mdtest -d /data/mysql/bfs -z 1 -i 1 -I 1