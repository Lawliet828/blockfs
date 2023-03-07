#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os, re, subprocess

log_path = "/data/mysql/log/"
bfs_path = "/data/mysql/bfs/"
stubs = ["open_file_1"]
# TODO 生成测试结果文件

# 启动block_fs_mount进程
def start_mount():
    subprocess.check_call("sudo ../build/tool/block_fs_mount -c ../conf/bfs.cnf -m", shell=True)

# 判断进程是否启动正常
def check_mount():
    mount_info = subprocess.check_output("ps -ef | grep block_fs_mount", shell=True).decode()
    if -1 == mount_info.find("../build/tool/block_fs_mount"):
        print("block_fs_mount启动失败")

    dirs = os.listdir(log_path)
    # TODO 识别最新的log文件
    for file in dirs:
        if re.match(r'^block\_fs\_mount\..*\.log$', file):
            file = log_path + file
            with open(file, 'r') as f:
                content = f.read()
                if -1 == content.find("FUSE version: 3.10.3"):
                    print(content)
                    print("启动异常")
                else:
                    print("启动正常")

# 执行block_fs_stub.py脚本
def do_stub(stub_name):
    stub_result = subprocess.check_output(["sudo", "python3", "../tool/block_fs_stub.py", stub_name]).decode()
    if -1 == stub_result.find("success"):
        print("stub fail")
    else:
        print("stub success")

# 执行桩点对应的文件操作
# TODO 不同的桩点不同的操作
def operate(stub_name):
    # 查看当前工作目录
    ret = os.getcwd()
    print("当前工作目录为 %s" % ret)

    # 修改当前工作目录
    os.chdir(bfs_path)

    # 查看修改后的工作目录
    ret = os.getcwd()
    print("目录修改成功 %s" % ret)

    print(subprocess.call("cat 1.log", shell=True))


# 检查进程是否被kill
def is_killed():
    mount_info = subprocess.check_output("ps -ef | grep block_fs_mount", shell=True).decode()
    if -1 == mount_info.find("../build/tool/block_fs_mount"):
        print("block_fs_mount已经被kill")
    # TODO 不需要删除log文件
    subprocess.check_call("rm -f /data/mysql/log/block_fs_mount*", shell=True)

if __name__ == "__main__":
    for stub_name in stubs:
        start_mount()
        check_mount()
        do_stub(stub_name)
        operate(stub_name)
        is_killed()
