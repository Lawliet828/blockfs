#!/bin/bash

install_cmake() {
    if [ ! -f "cmake-3.20.3.tar.gz" ]; then
        wget https://cmake.org/files/v3.20/cmake-3.20.3.tar.gz
    fi
    tar -xvf cmake-3.20.3.tar.gz
    cd cmake-3.20.3/
    ./bootstrap
    gmake -j4
    gmake install
    ln -s /usr/local/bin/cmake /usr/bin/
}

echo "***** install cmake *****"
cmake_version=$(cmake --version | grep "cmake version" | awk '{print $3}')
if [ -z "$cmake_version" ]; then
    echo "cmake not installed."
    install_cmake
else
    echo "cmake version: $cmake_version"
fi

if grep -q 'ID=centos' /etc/os-release; then
    echo "Redhat/Centos detected."
    yum -y install python3
elif grep -q 'ID=debian' /etc/os-release; then
    echo "Debian Linux detected."
    apt install python3 -y
elif grep -q 'ID=ubuntu' /etc/os-release; then
    echo "Ubuntu Linux detected."
    sudo apt update
    sudo apt install autoconf automake make pkg-config -y
    sudo apt install libtool cmake curl gcc g++ unzip -y
    sudo apt install libaio-dev uuid-dev libc6-dev -y
    sudo apt install ninja-build meson -y
    sudo apt install python3 -y
    sudo apt install python3-pip -y
else
    echo "Unknown Linux distribution."
fi
pip3 install meson ninja

# libuuid项目地址 https://sourceforge.net/projects/libuuid/
# 去掉系统库,因为是动态库,尽量使用静态库
# if [[ "$OSTYPE" == "linux-gnu" ]]; then
#     if [ -f /etc/redhat-release ]; then
#         echo "Redhat/Centos detected."
#         yum remove libuuid-devel -y
#     elif [ -f /etc/debian_version ]; then
#         echo "Ubuntu/Debian Linux detected."
#         sudo apt-get remove uuid-dev
#     else
#         echo "Unknown Linux distribution."
#     fi
# fi

# if [ ! -f "libuuid-1.0.3.tar.gz" ]; then
#     wget https://nchc.dl.sourceforge.net/project/libuuid/libuuid-1.0.3.tar.gz
#     sudo tar -xvf libuuid-1.0.3.tar.gz
#     cd libuuid-1.0.3
#     sudo ./configure --prefix=/usr
#     sudo make -j8
#     sudo make install
#     cd ..
#     sudo rm -rf libuuid-1.0.3
# fi


if [ ! -d "googletest" ]; then
    git clone git@github.com:google/googletest.git
    sudo mkdir -p googletest/build
    cd googletest/build
    sudo cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
    sudo make -j8
    sudo make install
    cd ../..
    # sudo cp -rvf /usr/lib/x86_64-linux-gnu/libgtest* /usr/lib
    # sudo cp -rvf /usr/lib/x86_64-linux-gnu/libgmock* /usr/lib
fi

if [ ! -d "libfuse" ]; then
    git clone git@github.com:libfuse/libfuse.git
    cd libfuse
    mkdir build; cd build
    meson setup ..
    meson configure -D disable-mtab=true -D default_library=both
    sudo ninja
    sudo ninja install
    cd ../..
fi

cd ..
