# Block File System

## 1. Introduction
#### 基于快杰云盘快存储的用户态分布式文件系统，主要功能和特点：

  * 这是一个基于共享块存储的用户态文件系统，业界参考
    * Aurora
    * PloarFS
    * BlueFS
    * CynosDB
    * FastCFS
    * BlobFS
    
  * UXDB-UDISK共享存储
    * 实现计算节点与存储节点的分离，提供了即时生效的可扩展能力和运维能力
    * 计算节点: 主要做SQL解析以及存储引擎计算的服务器
    * 存储节点: 主要做数据块存储, 数据库快照的服务器
    * 与传统云数据库一个实例一份数据拷贝不同, UXDB同一个实例的所有节点(包括读写节点和只读节点)
    * 都实现访问存储节点上的同一份数据，使得UXDB的数据备份耗时实现秒级响应, 同时节约存储的成本

  * UXDB-UDISK快速迁移
    * 借助底层UDISK的RDMA网络以及最新的NVMe块存储技术, 实现服务器宕机后无需搬运数据重启进程即可服务,
    * 另外一旦主异常，多个从数据库也可以快速提供务 满足了互联网环境下企业对数据库服务器高可用的需求.
    * 数据库应用的持久状态可下移至分布式文件系统，由分布式存储提供较高的数据可用性和可靠性.
    * 因此数据库的高可用处理可被简化，也利于数据库实例在计算节点上灵活快速地迁移
  
  * UXDB-UDISK主从数据同步
    * 利用UDISK的底层资源实现共享存储,需要在在用户空间中实施类似POSIX的接口,新的API类似POSIX
    * BlockFS是一个轻量级的用户空间库,BlockFS采用编译到数据库的形态，替换标准的文件系统接口
    * 这会使得全部的I/O路径都在用户空间中,数据处理在用户空间完成,尽可能减少数据的拷贝
    * 这样做的目的是避免传统文件系统从内核空间至用户空间的消息传递开销,尤其数据拷贝的开销
    * 这对于低延迟硬件的性能发挥尤为重要,这让UXDB能够仅需少许改动即可提升性能。

## 2. Design Paper

* [BlockFS详细设计文档](https://git.ucloudadmin.com/block/udisk_doc/-/wikis/%E5%9F%BA%E4%BA%8E%E5%BF%AB%E6%9D%B0%E7%9A%84%E5%88%86%E5%B8%83%E5%BC%8F%E6%96%87%E4%BB%B6%E7%B3%BB%E7%BB%9Fblockfs%E8%AE%BE%E8%AE%A1%E6%96%87%E6%A1%A3)
* [运维工具链改造 - 需求文档-Final](https://ushare.ucloudadmin.com/pages/viewpage.action?pageId=66356907)
* [低成本高可用MySQL8内核-用户手册](https://ushare.ucloudadmin.com/pages/viewpage.action?pageId=66361244)
* [UXDB原型](https://ushare.ucloudadmin.com/pages/viewpage.action?pageId=57602155)


## 3. Quick Start

版本要求:
- gcc7或以上(支持c++17)
- CMake 3.0.2或以上
- python3, meson和ninja

**如果是ucloud Centos 7.6云主机, 可以执行如下脚本自动安装上述工具**
```
cd blockfs/deps
bash update_deps.sh
```

#### 3.1. 安装依赖的库

```sh
cd blockfs/deps

sudo ./make_deps.sh
```

#### 3.2. 编译libblock_fs.a

```sh
./do_cmake.sh

cd build

make -j8
```

编译会生成静态库和工具以及测试程序


#### 3.3 tool: block_fs_tool

[block_fs_tool工具挂载使用文档](doc/block_fs_tools.md)


#### 3.4. tool: block_fs_mount

[block_fs_mount工具挂载使用文档](doc/block_fs_mount.md)

[block_fs支持FUSE挂载的相关文档](doc/block_fs_fuse.md)


## 4. Testing


#### 4.1. mysql模拟测试工具

```sh

git clone https://git.ucloudadmin.com/udb/db_source_code/blockfs_tools

```


#### 4.2. 性能压测工具

```sh
cd deps/dep_test

luotang@10-23-227-66:~/blockfs/deps/dep_test$ ll
total 1868
drwxrwxr-x 6 luotang luotang    4096 Feb  1 14:52 ./
drwxrwxr-x 7 luotang luotang    4096 Feb  1 15:13 ../
drwxrwxr-x 7 luotang luotang    4096 Jan 27 11:39 blockfs_tools/
drwxrwxr-x 5 luotang luotang    4096 Jan 29 18:23 file-system-stress-testing/
drwxrwxr-x 7 luotang luotang    4096 Jan 29 18:26 ior/
-rwxrwxr-x 1 luotang luotang 1873920 Feb  1 14:48 iozone3_489.tar*
drwxrwxr-x 7 luotang luotang    4096 Jan 31 20:41 pjdfstest/
-rwxrwxr-x 1 luotang luotang    8605 Feb  1 14:48 README.md*
luotang@10-23-227-66:~/blockfs/deps/dep_test$ 

```

[压测工具介绍和使用](deps/dep_test/README.md)

#### 4.3. 一致性工具


## Tutorial

* [第1步：block_fs_test.cc](test/block_fs_test.cc)
* [第2步：block_fs_posix.cc](test/bfs_posix_test/block_fs_posix.cc)

## Benchmark

* CPU 2-chip/8-core/32-processor Intel(R) Xeon(R) CPU E5-2630 v3 @2.40GHz
* Memory all 128G
* 10 Gigabit Ethernet
