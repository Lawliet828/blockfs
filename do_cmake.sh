#!/bin/bash

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=1 "$@" ..
make -j8
