// test_performance.cpp
#include "simulation_manager.h"
#include <chrono>
#include <iostream>
#include <vector>

class PerformanceTester {
public:
  void run_performance_test() {
    std::cout << "=== 性能测试 ===" << std::endl;

    test_initialization_performance();
    test_connection_performance();
    test_message_throughput();
  }

private:
  void test_initialization_performance() {
    std::cout << "\n--- 初始化性能测试 ---" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    SimulationManager simulator;
    bool success =
        simulator.initialize("192.168.198.129", 9000, "localhost", "root",
                             "509876.zxn", "equipment_management");

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    if (success) {
      std::cout << "✅ 初始化完成，耗时: " << duration.count() << "ms"
                << std::endl;
    } else {
      std::cout << "❌ 初始化失败" << std::endl;
    }
  }

  void test_connection_performance() {
    std::cout << "\n--- 连接性能测试 ---" << std::endl;

    SimulationManager simulator;
    if (!simulator.initialize("192.168.198.129", 9000, "localhost", "root",
                              "509876.zxn", "equipment_management")) {
      std::cerr << "初始化失败，跳过连接性能测试" << std::endl;
      return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 测试连接所有设备
    bool success = simulator.connect_all_equipments();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    if (success) {
      std::cout << "✅ 连接所有设备完成，耗时: " << duration.count() << "ms"
                << std::endl;
    } else {
      std::cout << "❌ 连接设备失败" << std::endl;
    }

    // 清理
    simulator.disconnect_all_equipments();
  }

  void test_message_throughput() {
    std::cout << "\n--- 消息吞吐量测试 ---" << std::endl;

    // 这个测试需要运行中的模拟器
    std::cout << "消息吞吐量测试需要在运行中的模拟器进行" << std::endl;
    std::cout << "请运行基础功能测试并观察日志输出" << std::endl;
  }
};

int main() {
  PerformanceTester tester;
  tester.run_performance_test();
  return 0;
}