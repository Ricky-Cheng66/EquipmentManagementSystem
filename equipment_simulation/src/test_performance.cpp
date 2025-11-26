// test_performance.cpp
#include "simulation_manager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
  std::cout << "\n收到终止信号 (" << signal << ")，正在停止测试..."
            << std::endl;
  g_running = false;
}

class PerformanceTester {
private:
  std::unique_ptr<SimulationManager> simulator_;

public:
  void test_initialization_performance() {
    std::cout << "=== 初始化性能测试 ===" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    simulator_ = std::make_unique<SimulationManager>();
    bool success =
        simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
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

    simulator_.reset();
  }

  void test_connection_performance() {
    std::cout << "\n=== 连接性能测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "初始化失败，跳过连接性能测试" << std::endl;
      return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 测试连接所有设备
    bool success = simulator_->connect_all_equipments();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    if (success) {
      std::cout << "✅ 连接所有设备完成，耗时: " << duration.count() << "ms"
                << std::endl;
      simulator_->print_status();
    } else {
      std::cout << "❌ 连接设备失败" << std::endl;
    }

    // 清理
    simulator_->disconnect_all_equipments();
    simulator_.reset();
  }

  void test_message_throughput() {
    std::cout << "\n=== 消息吞吐量测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "初始化失败，跳过消息吞吐量测试" << std::endl;
      return;
    }

    // 启动模拟器
    if (!simulator_->start()) {
      std::cerr << "模拟器启动失败" << std::endl;
      return;
    }

    std::cout << "消息吞吐量测试运行中，按 Ctrl+C 停止..." << std::endl;

    int message_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    // 运行测试一段时间
    while (g_running) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = current_time - start_time;

      if (elapsed >= std::chrono::seconds(30)) {
        std::cout << "测试时间到达，停止测试..." << std::endl;
        break;
      }

      // 每5秒打印一次状态
      auto elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      if (elapsed_seconds % 5 == 0) {
        std::cout << "已运行 " << elapsed_seconds
                  << " 秒，消息计数: " << message_count << std::endl;
        simulator_->print_status();
      }

      // 短暂休眠
      for (int i = 0; i < 10 && g_running; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      message_count++;
    }

    // 计算性能指标
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration =
        std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    if (total_duration.count() > 0) {
      double messages_per_second =
          static_cast<double>(message_count) / total_duration.count();
      std::cout << "✅ 消息吞吐量测试完成:" << std::endl;
      std::cout << "   总消息数: " << message_count << std::endl;
      std::cout << "   总时间: " << total_duration.count() << "秒" << std::endl;
      std::cout << "   平均吞吐量: " << messages_per_second << " 消息/秒"
                << std::endl;
    }

    simulator_->stop();
    simulator_.reset();
  }

  void run_all_tests() {
    test_initialization_performance();

    if (g_running) {
      test_connection_performance();
    }

    if (g_running) {
      test_message_throughput();
    }
  }
};

int main() {
  // 注册信号处理
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "开始性能测试..." << std::endl;
  std::cout << "服务器地址: 192.168.198.129:9000" << std::endl;
  std::cout << "测试将在30秒后自动结束，或按Ctrl+C手动结束" << std::endl;

  PerformanceTester tester;

  try {
    tester.run_all_tests();
    std::cout << "\n🎉 性能测试完成!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "❌ 性能测试过程中发生异常: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "测试程序退出" << std::endl;
  return 0;
}