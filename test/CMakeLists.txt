
set(COMMLIBS block_fs gtest.a gtest_main.a)

# 2. IO NoMerges验证测试
add_executable(block_fs_ionomerges block_fs_ionomerges.cc)
target_link_libraries(block_fs_ionomerges ${COMMLIBS})

# 4. Posix 空洞验证
add_executable(block_fs_hole block_fs_hole.cc)
target_link_libraries(block_fs_hole ${COMMLIBS})

add_executable(io_test io_test.cc)
target_link_libraries(io_test aio event)
