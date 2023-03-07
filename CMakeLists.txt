cmake_minimum_required(VERSION 2.8)
project(BlockFS)

set(BLOCK_FS_VERSION_MAJOR 0)
set(BLOCK_FS_VERSION_MINOR 1)

MESSAGE( STATUS "Build CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

include (${CMAKE_SOURCE_DIR}/deps/cmake/utils.cmake)

include_directories(/usr/include/fuse3)
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/tool)
link_directories(${CMAKE_SOURCE_DIR}/build/block_fs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O2 -Wall -fPIC")

add_definitions(-Wno-builtin-macro-redefined)

execute_process(COMMAND date +%y.%m.%d OUTPUT_VARIABLE DATEVER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log --pretty=format:%h -1 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GITVER)

execute_process(COMMAND uname -m OUTPUT_VARIABLE MACHINE_VER)
string(REGEX REPLACE "\n$" "" MACHINE_VER "${MACHINE_VER}")

MESSAGE(STATUS "Detecting build MACHINE_VER: ${MACHINE_VER}")

set(BLOCK_FS_VERSION ${DATEVER}-${GITVER})

add_definitions(-DVERSION_TAG=${BLOCK_FS_VERSION})

add_subdirectory(lib)
add_subdirectory(tool)
add_subdirectory(test)
