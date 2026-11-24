# 设定cmake版本的最低要求
cmake_minimum_required(VERSION 3.5)

# 设定编译工具链：交叉编译器 arm-linux-gcc
set(CMAKE_C_COMPILER "/usr/local/arm/5.4.0/usr/bin/arm-linux-gcc")

# 设定项目名称、版本、语言
project(car VERSION 1.0.0 LANGUAGES C)

# 编译选项：打开警告
set(CMAKE_C_FLAGS "-Wall")

# 添加头文件目录（inc 是功能头文件，log 是日志模块头文件）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../inc
                    ${CMAKE_CURRENT_SOURCE_DIR}/../log)

# 收集src下所有.c源文件，外加日志模块 log/log.c
file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c
              ${CMAKE_CURRENT_SOURCE_DIR}/../log/log.c)

# 生成可执行文件a.out
add_executable(a.out ${SRC})

# 链接库，-lpthread -lm
target_link_libraries(a.out pthread m)
