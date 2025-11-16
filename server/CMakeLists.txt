cmake_minimum_required(VERSION 3.10)
project(EMS LANGUAGES CXX)

# 语言标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源码
file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# 可执行文件
add_executable(EMS_server ${SRCS})

# 头文件路径
target_include_directories(EMS_server PRIVATE
    ${CMAKE_SOURCE_DIR}/shared_components/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 查找 MySQL 库
find_library(MYSQL_LIB mysqlclient)

if(NOT MYSQL_LIB)
    message(FATAL_ERROR "MySQL client library not found")
endif()

# 链接到目标
target_link_libraries(EMS_server shared_components ${MYSQL_LIB})

# 可选：常用编译警告/优化
target_compile_options(EMS_server PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
