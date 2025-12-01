#include "simulation_manager.h"
#include <iostream>
#include <unistd.h>

int main() {
  std::cout << "=== 设备模拟器启动 ===" << std::endl;

  // 创建模拟管理器
  SimulationManager simulator;

  // 初始化模拟器
  if (!simulator.initialize("127.0.0.1", 9000, "localhost", "root",
                            "509876.zxn", "equipment_management", 3306)) {
    std::cout << "模拟器初始化失败!" << std::endl;
    return -1;
  }

  // 启动模拟器
  if (!simulator.start()) {
    std::cout << "模拟器启动失败!" << std::endl;
    return -1;
  }

  std::cout << "设备模拟器运行中..." << std::endl;

  // 等待用户停止
  std::cin.get();

  std::cout << "模拟器正在停止..." << std::endl;

  return 0;
}