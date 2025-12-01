cmake_minimum_required(VERSION 3.10)
project(EquipmentManagementSystem LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加服务器子目录
add_subdirectory(shared_components)
add_subdirectory(equipment_simulation)
add_subdirectory(test)
add_subdirectory(server)

# 后续添加设备模拟和Qt客户端

# add_subdirectory(qt-client)