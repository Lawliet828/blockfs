cmake_minimum_required(VERSION 3.11)
project(BlockFS)

set(BLOCK_FS_VERSION_MAJOR 0)
set(BLOCK_FS_VERSION_MINOR 1)

MESSAGE(STATUS "Build CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

include (${CMAKE_SOURCE_DIR}/deps/cmake/utils.cmake)

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

# pkg-config --cflags-only-I fuse3
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(FUSE3 REQUIRED fuse3)
# message(STATUS "FUSE3_INCLUDE_DIRS: ${FUSE3_INCLUDE_DIRS}")
# message(STATUS "FUSE3_LIBRARY_DIRS: ${FUSE3_LIBRARY_DIRS}")
# message(STATUS "FUSE3_LIBRARIES: ${FUSE3_LIBRARIES}")
# include_directories(${FUSE3_INCLUDE_DIRS})
include_directories(/usr/local/include/fuse3)
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/tool)
include_directories(${CMAKE_SOURCE_DIR}/token_bucket)
link_directories(${CMAKE_SOURCE_DIR}/build/block_fs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g -O2 -Wall -fPIC")

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
add_subdirectory(token_bucket)
