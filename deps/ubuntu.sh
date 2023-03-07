sudo apt update

sudo apt install autoconf automake make pkg-config -y
sudo apt install libtool cmake curl gcc g++ unzip -y
sudo apt install libaio-dev uuid-dev libc6-dev -y
sudo apt install ninja-build meson -y

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
    meson --prefix=/usr build
    cd build
    meson configure -D disable-mtab=true
    sudo ninja
    sudo ninja install
    cd ../..
fi