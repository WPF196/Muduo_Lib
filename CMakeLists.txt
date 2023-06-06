# mymuduo最终编译成so动态库

cmake_minimum_required(VERSION 2.5) # 最小支持版本
project(mymuduo)                    # 项目名

# 设置动态库的输出路径（根目录的lib文件夹下）
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息 以及 启动c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 定义参与编译的源文件（当前目录全部文件统定义为SRC_LIST）
aux_source_directory(. SRC_LIST) 
# 编译生成动态库libmymuduo.so
add_library(mymuduo SHARED ${SRC_LIST})