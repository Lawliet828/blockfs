
set(COMMLIBS block_fs uuid)
set(EXECUTABLE_NAME "block_fs_tool")

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

# touch工具
add_library(block_fs_touch_objs OBJECT
            block_fs_touch.cc)
redefine_file_macro(block_fs_touch_objs)
add_executable(block_fs_touch $<TARGET_OBJECTS:block_fs_touch_objs>)
target_link_libraries(block_fs_touch ${COMMLIBS})
set_target_properties(block_fs_touch PROPERTIES VERSION ${BLOCK_FS_VERSION})


# unlink工具
add_library(block_fs_unlink_objs OBJECT
            block_fs_unlink.cc)
redefine_file_macro(block_fs_unlink_objs)
add_executable(block_fs_unlink $<TARGET_OBJECTS:block_fs_unlink_objs>)
target_link_libraries(block_fs_unlink ${COMMLIBS})
set_target_properties(block_fs_unlink PROPERTIES VERSION ${BLOCK_FS_VERSION})

# ls工具
add_library(block_fs_ls_objs OBJECT
            block_fs_ls.cc)
redefine_file_macro(block_fs_ls_objs)
add_executable(block_fs_ls $<TARGET_OBJECTS:block_fs_ls_objs>)
target_link_libraries(block_fs_ls ${COMMLIBS})
set_target_properties(block_fs_ls PROPERTIES VERSION ${BLOCK_FS_VERSION})


# extract_device工具
add_library(block_fs_extract_device_objs OBJECT
            block_fs_extract_device.cc)
redefine_file_macro(block_fs_extract_device_objs)
add_executable(block_fs_extract_device $<TARGET_OBJECTS:block_fs_extract_device_objs>)
target_link_libraries(block_fs_extract_device ${COMMLIBS})
set_target_properties(block_fs_extract_device PROPERTIES VERSION ${BLOCK_FS_VERSION})

