
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

# mkdir工具
add_library(block_fs_mkdir_objs OBJECT
            block_fs_mkdir.cc)
redefine_file_macro(block_fs_mkdir_objs)
add_executable(block_fs_mkdir $<TARGET_OBJECTS:block_fs_mkdir_objs>)
target_link_libraries(block_fs_mkdir ${COMMLIBS})
set_target_properties(block_fs_mkdir PROPERTIES VERSION ${BLOCK_FS_VERSION})

# rmdir工具
add_library(block_fs_rmdir_objs OBJECT
            block_fs_rmdir.cc)
redefine_file_macro(block_fs_rmdir_objs)
add_executable(block_fs_rmdir $<TARGET_OBJECTS:block_fs_rmdir_objs>)
target_link_libraries(block_fs_rmdir ${COMMLIBS})
set_target_properties(block_fs_rmdir PROPERTIES VERSION ${BLOCK_FS_VERSION})


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

# rm工具
add_library(block_fs_rm_objs OBJECT
            block_fs_rm.cc)
redefine_file_macro(block_fs_rm_objs)
add_executable(block_fs_rm $<TARGET_OBJECTS:block_fs_rm_objs>)
target_link_libraries(block_fs_rm ${COMMLIBS})
set_target_properties(block_fs_rm PROPERTIES VERSION ${BLOCK_FS_VERSION})


# cat工具
add_library(block_fs_cat_objs OBJECT
            block_fs_cat.cc)
redefine_file_macro(block_fs_cat_objs)
add_executable(block_fs_cat $<TARGET_OBJECTS:block_fs_cat_objs>)
target_link_libraries(block_fs_cat ${COMMLIBS})
set_target_properties(block_fs_cat PROPERTIES VERSION ${BLOCK_FS_VERSION})


# rename工具
add_library(block_fs_rename_objs OBJECT
            block_fs_rename.cc)
redefine_file_macro(block_fs_rename_objs)
add_executable(block_fs_rename $<TARGET_OBJECTS:block_fs_rename_objs>)
target_link_libraries(block_fs_rename ${COMMLIBS})
set_target_properties(block_fs_rename PROPERTIES VERSION ${BLOCK_FS_VERSION})


# mv工具
add_library(block_fs_mv_objs OBJECT
            block_fs_mv.cc)
redefine_file_macro(block_fs_mv_objs)
add_executable(block_fs_mv $<TARGET_OBJECTS:block_fs_mv_objs>)
target_link_libraries(block_fs_mv ${COMMLIBS})
set_target_properties(block_fs_mv PROPERTIES VERSION ${BLOCK_FS_VERSION})

# ls工具
add_library(block_fs_ls_objs OBJECT
            block_fs_ls.cc)
redefine_file_macro(block_fs_ls_objs)
add_executable(block_fs_ls $<TARGET_OBJECTS:block_fs_ls_objs>)
target_link_libraries(block_fs_ls ${COMMLIBS})
set_target_properties(block_fs_ls PROPERTIES VERSION ${BLOCK_FS_VERSION})


# cp工具
add_library(block_fs_cp_objs OBJECT
            block_fs_cp.cc)
redefine_file_macro(block_fs_cp_objs)
add_executable(block_fs_cp $<TARGET_OBJECTS:block_fs_cp_objs>)
target_link_libraries(block_fs_cp ${COMMLIBS})
set_target_properties(block_fs_cp PROPERTIES VERSION ${BLOCK_FS_VERSION})

# cp_range工具
add_library(block_fs_cp_range_objs OBJECT
            block_fs_cp_range.cc)
redefine_file_macro(block_fs_cp_range_objs)
add_executable(block_fs_cp_range $<TARGET_OBJECTS:block_fs_cp_range_objs>)
target_link_libraries(block_fs_cp_range ${COMMLIBS})
set_target_properties(block_fs_cp_range PROPERTIES VERSION ${BLOCK_FS_VERSION})

# extract_device工具
add_library(block_fs_extract_device_objs OBJECT
            block_fs_extract_device.cc)
redefine_file_macro(block_fs_extract_device_objs)
add_executable(block_fs_extract_device $<TARGET_OBJECTS:block_fs_extract_device_objs>)
target_link_libraries(block_fs_extract_device ${COMMLIBS})
set_target_properties(block_fs_extract_device PROPERTIES VERSION ${BLOCK_FS_VERSION})

