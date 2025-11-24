// test_simulation_manager_fixed.cpp
#include "simulation_manager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

std::atomic<bool> g_running{true};
std::atomic<bool> g_stop_requested{false};

void signal_handler(int signal) {
  std::cout << "\n收到终止信号 (" << signal << ")，正在关闭模拟器..."
            << std::endl;
  g_running = false;
  g_stop_requested = true;
}

class SimulationManagerTester {
private:
  std::unique_ptr<SimulationManager> simulator_;

public:
  void test_basic_functionality() {
    std::cout << "=== 基础功能测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();

    // 初始化模拟器
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "❌ 模拟器初始化失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器初始化成功" << std::endl;

    // 设置更短的测试时间
    std::cout << "模拟器将运行30秒进行测试..." << std::endl;

    // 启动模拟器
    if (!simulator_->start()) {
      std::cerr << "❌ 模拟器启动失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器启动成功" << std::endl;
    std::cout << "按 Ctrl+C 可提前停止测试" << std::endl;

    // 运行测试，但定期检查停止信号
    auto start_time = std::chrono::steady_clock::now();
    auto test_duration = std::chrono::seconds(30);

    while (g_running) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = current_time - start_time;

      if (elapsed >= test_duration) {
        std::cout << "测试时间到达，停止模拟器..." << std::endl;
        break;
      }

      // 使用更短的检查间隔，提高响应性
      for (int i = 0; i < 10 && g_running; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      // 定期打印状态
      auto elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      if (elapsed_seconds % 10 == 0) {
        std::cout << "已运行 " << elapsed_seconds << " 秒..." << std::endl;
        if (simulator_->is_running()) {
          simulator_->print_status();
        }
      }
    }

    // 停止模拟器
    std::cout << "正在停止模拟器..." << std::endl;
    simulator_->stop();
    std::cout << "✅ 模拟器测试完成" << std::endl;
    simulator_.reset(); // 确保资源释放
  }

  void test_quick_connection() {
    std::cout << "\n=== 快速连接测试 ===" << std::endl;

    auto simulator = std::make_unique<SimulationManager>();

    if (!simulator->initialize("192.168.198.129", 9000, "localhost", "root",
                               "509876.zxn", "equipment_management")) {
      std::cerr << "❌ 模拟器初始化失败" << std::endl;
      return;
    }

    // 只测试连接，不启动事件循环
    std::cout << "测试设备连接..." << std::endl;
    bool success = simulator->connect_all_equipments();

    if (success) {
      std::cout << "✅ 设备连接测试通过" << std::endl;
      simulator->print_status();
    } else {
      std::cout << "❌ 设备连接测试失败" << std::endl;
    }

    // 立即断开所有连接
    simulator->disconnect_all_equipments();
    std::cout << "✅ 快速连接测试完成" << std::endl;
  }

  void test_manual_control() {
    std::cout << "\n=== 手动控制测试 ===" << std::endl;

    auto simulator = std::make_unique<SimulationManager>();

    if (!simulator->initialize("192.168.198.129", 9000, "localhost", "root",
                               "509876.zxn", "equipment_management")) {
      std::cerr << "❌ 模拟器初始化失败" << std::endl;
      return;
    }

    // 测试单个设备连接
    std::cout << "测试单个设备连接: real_proj_001" << std::endl;
    bool connected = simulator->connect_equipment("real_proj_001");

    if (connected) {
      std::cout << "✅ 单个设备连接成功" << std::endl;

      // 等待2秒看连接状态
      std::this_thread::sleep_for(std::chrono::seconds(2));
      simulator->print_status();

      // 断开设备
      std::cout << "断开设备..." << std::endl;
      simulator->disconnect_equipment("real_proj_001");
      std::cout << "✅ 设备断开成功" << std::endl;
    } else {
      std::cout << "❌ 单个设备连接失败" << std::endl;
    }

    std::cout << "✅ 手动控制测试完成" << std::endl;
  }
};

int main() {
  // 注册信号处理
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "开始 SimulationManager 修复测试..." << std::endl;
  std::cout << "服务器地址: 192.168.198.129:9000" << std::endl;
  std::cout << "测试将在30秒后自动结束，或按Ctrl+C手动结束" << std::endl;

  SimulationManagerTester tester;

  try {
    // 运行基础功能测试（有限时间）
    tester.test_basic_functionality();

    // 如果用户没有请求停止，继续其他测试
    if (!g_stop_requested) {
      tester.test_quick_connection();
      tester.test_manual_control();

      std::cout << "\n🎉 所有测试完成!" << std::endl;
    } else {
      std::cout << "\n测试被用户中断" << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "❌ 测试过程中发生异常: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "测试程序退出" << std::endl;
  return 0;
}