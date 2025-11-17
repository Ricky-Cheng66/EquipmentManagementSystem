cmake_minimum_required(VERSION 3.10)
project(EquipmentStimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
file(GLOB_RECURSE SRCS src/*.cpp)

# 可执行文件
add_executable(equipment_stimulation ${SRCS})

# 头文件路径
target_include_directories(equipment_stimulation PRIVATE
    ${CMAKE_SOURCE_DIR}/shared_components/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接共享组件库
target_link_libraries(equipment_stimulation shared_components)

# 编译选项
target_compile_options(equipment_stimulation PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)