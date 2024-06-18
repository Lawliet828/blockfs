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

echo "***** install necessary libraries *****"
if grep -q 'ID=centos' /etc/os-release || grep -q 'ID=debian' /etc/os-release; then
    echo "Redhat/Centos or Debian Linux detected."
    yum -y install autoconf automake curl gcc gcc-c++ make git libtool vim
    yum -y install epel-release wget bzip2-devel zlib-devel
    yum -y install libaio-devel libevent-devel libffi-devel
    yum -y groupinstall "Development Tools"
    yum -y install python3
elif grep -q 'ID=ubuntu' /etc/os-release; then
    echo "Ubuntu Linux detected."
    sudo apt update
    sudo apt install autoconf automake cmake curl gcc g++ make pkg-config libtool unzip -y
    sudo apt install libaio-dev libc6-dev libarchive-zip-perl libsnappy-dev libssl-dev libsasl2-dev libevent-dev zlib1g-dev uuid-dev -y
    sudo apt install ninja-build meson google-perftools -y
    sudo apt install python3 -y
    sudo apt install python3-pip -y
else
    echo "Unknown Linux distribution."
fi
pip3 install meson ninja


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
    ln -s /usr/local/lib64/libfuse3.so.3 /usr/lib64/libfuse3.so.3
    cd ../..
fi

if [ ! -d "spdlog" ]; then
    git clone https://github.com/gabime/spdlog.git
    cd spdlog && mkdir build && cd build
    cmake .. && make -j
    sudo make install
    cd ../..
fi

cd ..
