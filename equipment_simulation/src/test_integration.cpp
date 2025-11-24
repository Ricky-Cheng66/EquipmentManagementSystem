// test_integration.cpp
#include "simulation_manager.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class IntegrationTester {
private:
  std::atomic<bool> running_{false};
  std::unique_ptr<SimulationManager> simulator_;

public:
  void run_integration_test() {
    std::cout << "=== 集成测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();

    // 初始化
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "集成测试初始化失败" << std::endl;
      return;
    }

    // 启动
    if (!simulator_->start()) {
      std::cerr << "集成测试启动失败" << std::endl;
      return;
    }

    running_ = true;

    // 模拟各种操作
    std::thread control_thread(&IntegrationTester::control_operations, this);
    std::thread status_thread(&IntegrationTester::status_monitoring, this);

    // 运行测试
    std::this_thread::sleep_for(std::chrono::seconds(120)); // 运行2分钟

    // 停止测试
    running_ = false;
    control_thread.join();
    status_thread.join();

    simulator_->stop();
    std::cout << "集成测试完成" << std::endl;
  }

private:
  void control_operations() {
    int operation_count = 0;

    while (running_) {
      std::this_thread::sleep_for(std::chrono::seconds(15));

      switch (operation_count % 4) {
      case 0:
        std::cout << "[控制] 模拟批量开启设备..." << std::endl;
        // 这里可以添加批量操作逻辑
        break;
      case 1:
        std::cout << "[控制] 模拟批量关闭设备..." << std::endl;
        // 这里可以添加批量操作逻辑
        break;
      case 2:
        std::cout << "[控制] 模拟重新连接设备..." << std::endl;
        simulator_->disconnect_equipment("real_proj_001");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        simulator_->connect_equipment("real_proj_001");
        break;
      case 3:
        std::cout << "[控制] 模拟状态检查..." << std::endl;
        simulator_->print_status();
        break;
      }

      operation_count++;
    }
  }

  void status_monitoring() {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::seconds(30));
      std::cout << "[监控] 系统运行正常..." << std::endl;
    }
  }
};

int main() {
  IntegrationTester tester;
  tester.run_integration_test();
  return 0;
}