cmake_minimum_required(VERSION 3.10)
project(EMS LANGUAGES CXX)

# 语言标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源码
file(GLOB_RECURSE SRCS src/*.cpp)

# 可执行文件
add_executable(EMS_server ${SRCS})

# 头文件路径
target_include_directories(EMS_server PRIVATE include)

# 查找 MySQL 库
find_library(MYSQL_LIB mysqlclient)

if(NOT MYSQL_LIB)
    message(FATAL_ERROR "MySQL client library not found")
endif()

# 链接到目标
target_link_libraries(EMS_server ${MYSQL_LIB})

# 可选：常用编译警告/优化
target_compile_options(EMS_server PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# ---------------- 自定义 clean-all（可选）----------------
# 用法：cmake --build . --target clean-all
# 作用：删除整个构建目录，比内置 clean 更彻底
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}"
    COMMENT "Removing entire build directory..."
    VERBATIM
)
