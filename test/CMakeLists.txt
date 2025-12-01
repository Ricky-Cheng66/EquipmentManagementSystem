cmake_minimum_required(VERSION 3.10)
project(EquipmentStimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 主程序可执行文件
add_executable(test_heartbeat
    src/test_heartbeat.cpp
)

find_package(Threads REQUIRED)

# 链接到目标
target_link_libraries(test_heartbeat
    Threads::Threads)

# 可选：常用编译警告/优化
target_compile_options(test_heartbeat PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)