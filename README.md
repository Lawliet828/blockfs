# Block File System

## 1. Introduction

用户态分布式文件系统，主要功能和特点：

  * 这是一个基于共享块存储的用户态文件系统，业界参考
    * Aurora
    * PloarFS
    * BlueFS
    * CynosDB
    * FastCFS
    * BlobFS

## 3. Quick Start

版本要求:
- gcc11或以上(支持c++20)
- CMake 3.0.2或以上
- python3, meson和ninja

#### 3.1. 安装依赖的库

yum -y install gcc gcc-c++ make git automake libaio-devel openssl openssl-devel

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
