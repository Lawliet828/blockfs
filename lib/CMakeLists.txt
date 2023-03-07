aux_source_directory(. LIB_SRC_FILES)
add_library(block_fs STATIC ${LIB_SRC_FILES})
target_link_libraries(block_fs fuse3 uuid pthread rt)
redefine_file_macro(block_fs)

#install(TARGETS block_fs LIBRARY DESTINATION .)