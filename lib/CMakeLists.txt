aux_source_directory(. LIB_SRC_FILES)
add_library(block_fs STATIC ${LIB_SRC_FILES})

# 查找 zlib 库
find_package(ZLIB REQUIRED)

target_link_libraries(block_fs fuse3 uuid pthread rt ${ZLIB_LIBRARIES})

# 如果 zlib 库的头文件不在标准的头文件搜索路径中，你还需要添加头文件的搜索路径
target_include_directories(block_fs PRIVATE ${ZLIB_INCLUDE_DIRS})

redefine_file_macro(block_fs)

#install(TARGETS block_fs LIBRARY DESTINATION .)