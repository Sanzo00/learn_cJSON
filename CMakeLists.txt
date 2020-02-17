# CMake的最低版本
cmake_minimum_required( VERSION 2.8 )

# 指定项目名称, 通常对应项目文件夹
project(cJSON)

# 指定引用目录
include_directories( src )

# 自动构建源文件目录
aux_source_directory( test DIR_TEST )

# 添加子目录
add_subdirectory(src)

#message("${CMAKE_CURRENT_SOURCE_DIR}")

# 把test下所有的源文件的相对位置保存到all中
file(GLOB_RECURSE all RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/test/*.c")

# 遍历all中的源文件, 编译为对应的可执行文件
foreach(files IN LISTS all)
#	message(${files})
	# 获取文件名
	get_filename_component(filename ${files} NAME_WE)
	# 指定编译目标文件
	add_executable(${filename} ${files})
	# 链接库
	target_link_libraries(${filename} m CJSON)
endforeach()

#add_executable(print test/print_json.c)
#target_link_libraries(print m CJSON)
#add_executable(new test/new_json.c)
#target_link_libraries(new m CJSON)
