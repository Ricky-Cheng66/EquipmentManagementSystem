// test_integration.cpp
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

class IntegrationTester {
private:
  std::unique_ptr<SimulationManager> simulator_;
  std::atomic<bool> test_running_{false};

public:
  void run_long_running_test() {
    std::cout << "=== 长时间运行测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();

    // 初始化模拟器
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "❌ 模拟器初始化失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器初始化成功" << std::endl;

    // 启动模拟器
    if (!simulator_->start()) {
      std::cerr << "❌ 模拟器启动失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器启动成功" << std::endl;
    std::cout << "长时间运行测试开始，将运行5分钟，按 Ctrl+C 停止..."
              << std::endl;

    test_running_ = true;
    auto start_time = std::chrono::steady_clock::now();
    auto test_duration = std::chrono::minutes(5);

    int operation_count = 0;

    while (g_running && test_running_) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = current_time - start_time;

      // 检查测试时间是否到达
      if (elapsed >= test_duration) {
        std::cout << "测试时间到达，停止测试..." << std::endl;
        break;
      }

      // 每30秒执行一次操作
      auto elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      if (elapsed_seconds % 30 == 0) {
        perform_operation_cycle(operation_count);
        operation_count++;

        // 打印状态
        std::cout << "已运行 " << elapsed_seconds
                  << " 秒，操作周期: " << operation_count << std::endl;
        simulator_->print_status();
      }

      // 使用较短的检查间隔
      for (int i = 0; i < 10 && g_running && test_running_; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    // 停止模拟器
    std::cout << "正在停止模拟器..." << std::endl;
    simulator_->stop();
    std::cout << "✅ 长时间运行测试完成" << std::endl;
    simulator_.reset();
  }

  void run_stress_test() {
    std::cout << "\n=== 压力测试 ===" << std::endl;

    simulator_ = std::make_unique<SimulationManager>();

    // 初始化模拟器
    if (!simulator_->initialize("192.168.198.129", 9000, "localhost", "root",
                                "509876.zxn", "equipment_management")) {
      std::cerr << "❌ 模拟器初始化失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器初始化成功" << std::endl;

    // 启动模拟器
    if (!simulator_->start()) {
      std::cerr << "❌ 模拟器启动失败" << std::endl;
      return;
    }

    std::cout << "✅ 模拟器启动成功" << std::endl;
    std::cout << "压力测试开始，将运行2分钟，模拟高负载场景..." << std::endl;

    test_running_ = true;
    auto start_time = std::chrono::steady_clock::now();
    auto test_duration = std::chrono::minutes(2);

    int stress_cycle = 0;

    while (g_running && test_running_) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = current_time - start_time;

      // 检查测试时间是否到达
      if (elapsed >= test_duration) {
        std::cout << "测试时间到达，停止测试..." << std::endl;
        break;
      }

      // 每10秒执行一次压力操作
      auto elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      if (elapsed_seconds % 10 == 0) {
        perform_stress_operation(stress_cycle);
        stress_cycle++;

        // 打印状态
        std::cout << "压力测试已运行 " << elapsed_seconds
                  << " 秒，压力周期: " << stress_cycle << std::endl;
        if (stress_cycle % 3 == 0) {
          simulator_->print_status();
        }
      }

      // 使用较短的检查间隔
      for (int i = 0; i < 10 && g_running && test_running_; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    // 停止模拟器
    std::cout << "正在停止模拟器..." << std::endl;
    simulator_->stop();
    std::cout << "✅ 压力测试完成" << std::endl;
    simulator_.reset();
  }

private:
  void perform_operation_cycle(int cycle) {
    switch (cycle % 4) {
    case 0:
      std::cout << "[操作] 模拟设备状态轮询..." << std::endl;
      // 这里可以添加状态轮询逻辑
      break;
    case 1:
      std::cout << "[操作] 模拟设备控制命令..." << std::endl;
      // 这里可以添加控制命令逻辑
      break;
    case 2:
      std::cout << "[操作] 模拟设备重连..." << std::endl;
      // 断开并重新连接一个设备
      if (simulator_) {
        simulator_->disconnect_equipment("real_proj_001");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        simulator_->connect_equipment("real_proj_001");
      }
      break;
    case 3:
      std::cout << "[操作] 模拟系统维护..." << std::endl;
      // 这里可以添加系统维护逻辑
      break;
    }
  }

  void perform_stress_operation(int cycle) {
    switch (cycle % 3) {
    case 0:
      std::cout << "[压力] 模拟大量消息发送..." << std::endl;
      // 这里可以模拟大量消息发送
      break;
    case 1:
      std::cout << "[压力] 模拟频繁连接断开..." << std::endl;
      // 频繁连接断开设备
      if (simulator_) {
        for (int i = 0; i < 3 && g_running; i++) {
          simulator_->disconnect_equipment("real_camera_001");
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
          simulator_->connect_equipment("real_camera_001");
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
      }
      break;
    case 2:
      std::cout << "[压力] 模拟高负载状态更新..." << std::endl;
      // 这里可以模拟高负载状态更新
      break;
    }
  }
};

int main() {
  // 注册信号处理
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "开始集成测试..." << std::endl;
  std::cout << "服务器地址: 192.168.198.129:9000" << std::endl;
  std::cout << "测试包括长时间运行和压力测试" << std::endl;

  IntegrationTester tester;

  try {
    tester.run_long_running_test();

    if (g_running) {
      tester.run_stress_test();
    }

    std::cout << "\n🎉 集成测试完成!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "❌ 集成测试过程中发生异常: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "测试程序退出" << std::endl;
  return 0;
}