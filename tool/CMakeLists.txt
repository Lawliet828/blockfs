set(COMMLIBS block_fs)

# 基础工具
add_library(block_fs_tool_objs OBJECT
            block_fs_tool.cc)
redefine_file_macro(block_fs_tool_objs)
add_executable(block_fs_tool $<TARGET_OBJECTS:block_fs_tool_objs>)
target_link_libraries(block_fs_tool ${COMMLIBS})
set_target_properties(block_fs_tool PROPERTIES VERSION ${BLOCK_FS_VERSION})


# FUSE挂载工具
add_library(block_fs_mount_objs OBJECT
            block_fs_mount.cc)
redefine_file_macro(block_fs_mount_objs)
add_executable(block_fs_mount $<TARGET_OBJECTS:block_fs_mount_objs>)
target_link_libraries(block_fs_mount ${COMMLIBS})
set_target_properties(block_fs_mount PROPERTIES VERSION ${BLOCK_FS_VERSION})

# extract_device工具
add_library(block_fs_extract_device_objs OBJECT
            block_fs_extract_device.cc)
redefine_file_macro(block_fs_extract_device_objs)
add_executable(block_fs_extract_device $<TARGET_OBJECTS:block_fs_extract_device_objs>)
target_link_libraries(block_fs_extract_device ${COMMLIBS})
set_target_properties(block_fs_extract_device PROPERTIES VERSION ${BLOCK_FS_VERSION})

