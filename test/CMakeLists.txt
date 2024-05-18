
set(COMMLIBS block_fs gtest.a gtest_main.a)

# 1. Posix 接口兼容性测试
add_library(block_fs_posix_objs OBJECT
            block_fs_posix.cc)

add_executable(block_fs_posix $<TARGET_OBJECTS:block_fs_posix_objs>)
target_link_libraries(block_fs_posix ${COMMLIBS})

# 2. IO NoMerges验证测试
add_library(block_fs_ionomerges_objs OBJECT
            block_fs_ionomerges.cc)

add_executable(block_fs_ionomerges $<TARGET_OBJECTS:block_fs_ionomerges_objs>)
target_link_libraries(block_fs_ionomerges ${COMMLIBS})

# 4. Posix 空洞验证
add_library(block_fs_hole_objs OBJECT
            block_fs_hole.cc)

add_executable(block_fs_hole $<TARGET_OBJECTS:block_fs_hole_objs>)
target_link_libraries(block_fs_hole ${COMMLIBS})

# 创建文件夹 单元测试
add_library(test_mkdir_objs OBJECT
            test_mkdir.cc)

add_executable(test_mkdir $<TARGET_OBJECTS:test_mkdir_objs>)
target_link_libraries(test_mkdir ${COMMLIBS})

add_executable(io_test io_test.cc)
target_link_libraries(io_test aio event)