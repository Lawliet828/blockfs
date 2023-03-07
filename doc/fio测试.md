配置：

4核8G
40G RSSD数据盘

4K随机读写
```
fio -direct=1 -iodepth=32 -rw=randrw -ioengine=libaio -bs=4k -size=64M -numjobs=1 -runtime=300 -group_reporting -filename=/data/mysql/bfs/testfile -fallocate=none -name=Test_4K_RandRW -verify=md5


fio -direct=1 -iodepth=32 -rw=randrw -ioengine=libaio -bs=4k -size=64M -numjobs=1 -runtime=300 -group_reporting -filename=/opt/fastcfs/fuse/testfile -fallocate=none -name=Test_4K_RandRW -verify=md5
```

2023/03/07 11:40
   READ: bw=200KiB/s (204kB/s), 200KiB/s-200KiB/s (204kB/s-204kB/s), io=58.5MiB (61.3MB), run=299990-299990msec
  WRITE: bw=156KiB/s (159kB/s), 156KiB/s-156KiB/s (159kB/s-159kB/s), io=32.1MiB (33.7MB), run=211460-211460msec



## 优化项

1. 读写函数减少冗余的校验
2. 使用unlikey优化分支预测