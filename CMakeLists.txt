cmake_minimum_required(VERSION 3.10)
project(EMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS src/*.cpp)

add_executable(EMS_server ${SRCS})

target_include_directories(EMS_server PRIVATE include)
