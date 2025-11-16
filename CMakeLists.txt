cmake_minimum_required(VERSION 3.10)
project(EquipmentManagementSystem LANGUAGES CXX)

# 添加服务器子目录
add_subdirectory(server)

# 后续添加设备模拟和Qt客户端
# add_subdirectory(equipment-simulator)
# add_subdirectory(qt-client)