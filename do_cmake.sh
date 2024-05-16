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

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=1 "$@" ..
make -j8
