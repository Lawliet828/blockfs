set(COMMLIBS block_fs)

add_executable(token_bucket_test token_bucket_test.cc)
target_link_libraries(token_bucket_test ${COMMLIBS})