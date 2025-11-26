// test_simulator_connections.cpp
#include "simulator_connections.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
  std::cout << "\n收到终止信号 (" << signal << ")，正在停止测试..."
            << std::endl;
  g_running = false;
}

class SimulatorConnectionsTester {
private:
  std::unique_ptr<SimulatorConnections> connections_;

public:
  void test_database_operations() {
    std::cout << "=== 数据库操作测试 ===" << std::endl;

    connections_ = std::make_unique<SimulatorConnections>();

    // 测试从数据库加载设备信息
    bool success = connections_->initialize_from_database(
        "localhost", "root", "509876.zxn", "equipment_management");

    if (success) {
      std::cout << "✅ 数据库加载测试通过" << std::endl;
      connections_->print_statistics();
      connections_->print_all_equipments();
    } else {
      std::cout << "❌ 数据库加载测试失败" << std::endl;
    }
  }

  void test_connection_management() {
    std::cout << "\n=== 连接管理测试 ===" << std::endl;

    if (!connections_) {
      std::cerr << "连接管理器未初始化，跳过连接管理测试" << std::endl;
      return;
    }

    // 测试添加连接
    std::cout << "测试添加连接..." << std::endl;
    bool add1 = connections_->add_connection(100, "real_proj_001");
    bool add2 = connections_->add_connection(101, "real_ac_001");
    bool add3 = connections_->add_connection(102, "real_camera_001");

    if (add1 && add2 && add3) {
      std::cout << "✅ 添加连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 添加连接测试失败" << std::endl;
    }

    connections_->print_connections();

    // 测试重复添加
    std::cout << "测试重复添加连接..." << std::endl;
    bool duplicate = connections_->add_connection(100, "real_proj_001");
    if (!duplicate) {
      std::cout << "✅ 重复连接检测测试通过" << std::endl;
    } else {
      std::cout << "❌ 重复连接检测测试失败" << std::endl;
    }

    // 测试连接状态检查
    std::cout << "测试连接状态检查..." << std::endl;
    if (connections_->has_connection(100) &&
        connections_->is_equipment_connected("real_proj_001")) {
      std::cout << "✅ 连接状态检查测试通过" << std::endl;
    } else {
      std::cout << "❌ 连接状态检查测试失败" << std::endl;
    }

    // 测试设备查找
    std::cout << "测试设备查找..." << std::endl;
    auto equip1 = connections_->get_equipment_by_fd(100);
    auto equip2 = connections_->get_equipment_by_id("real_ac_001");
    int fd = connections_->get_fd_by_equipment_id("real_proj_001");

    if (equip1 && equip2 && fd == 100) {
      std::cout << "✅ 设备查找测试通过" << std::endl;
    } else {
      std::cout << "❌ 设备查找测试失败" << std::endl;
    }

    // 测试设备列表获取
    std::cout << "测试设备列表获取..." << std::endl;
    auto all_equipments = connections_->get_all_equipments();
    auto registered_equipments = connections_->get_registered_equipments();
    auto pending_equipments = connections_->get_pending_equipments();
    auto connected_equipments = connections_->get_connected_equipments();

    if (!all_equipments.empty() && !registered_equipments.empty() &&
        !pending_equipments.empty() && !connected_equipments.empty()) {
      std::cout << "✅ 设备列表获取测试通过" << std::endl;
    } else {
      std::cout << "❌ 设备列表获取测试失败" << std::endl;
    }

    // 测试状态管理
    std::cout << "测试状态管理..." << std::endl;
    bool status_updated =
        connections_->update_equipment_status("real_proj_001", "online");
    bool power_updated =
        connections_->update_equipment_power_state("real_proj_001", "on");

    if (status_updated && power_updated) {
      std::cout << "✅ 状态管理测试通过" << std::endl;
    } else {
      std::cout << "❌ 状态管理测试失败" << std::endl;
    }

    // 测试批量操作
    std::cout << "测试批量操作..." << std::endl;
    connections_->batch_update_status("offline");
    connections_->batch_update_power_state("off");
    std::cout << "✅ 批量操作测试完成" << std::endl;

    connections_->print_all_equipments();

    // 测试移除连接
    std::cout << "测试移除连接..." << std::endl;
    connections_->remove_connection(101);
    if (!connections_->has_connection(101) &&
        !connections_->is_equipment_connected("real_ac_001")) {
      std::cout << "✅ 移除连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 移除连接测试失败" << std::endl;
    }

    // 测试按设备ID移除连接
    std::cout << "测试按设备ID移除连接..." << std::endl;
    connections_->remove_connection_by_equipment_id("real_camera_001");
    if (!connections_->is_equipment_connected("real_camera_001")) {
      std::cout << "✅ 按设备ID移除连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 按设备ID移除连接测试失败" << std::endl;
    }

    std::cout << "移除后连接数: " << connections_->get_connection_count()
              << std::endl;
  }

  void test_error_conditions() {
    std::cout << "\n=== 错误条件测试 ===" << std::endl;

    if (!connections_) {
      std::cerr << "连接管理器未初始化，跳过错误条件测试" << std::endl;
      return;
    }

    // 测试无效FD
    std::cout << "测试无效FD..." << std::endl;
    bool invalid_fd = connections_->add_connection(-1, "real_proj_001");
    if (!invalid_fd) {
      std::cout << "✅ 无效FD处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 无效FD处理测试失败" << std::endl;
    }

    // 测试不存在的设备连接
    std::cout << "测试不存在的设备连接..." << std::endl;
    bool nonexistent_equipment =
        connections_->add_connection(300, "nonexistent_device");
    if (!nonexistent_equipment) {
      std::cout << "✅ 不存在设备连接处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 不存在设备连接处理测试失败" << std::endl;
    }

    // 测试移除不存在的连接
    std::cout << "测试移除不存在的连接..." << std::endl;
    connections_->remove_connection(999);
    connections_->remove_connection_by_equipment_id("nonexistent_device");
    std::cout << "✅ 移除不存在连接处理测试通过" << std::endl;

    // 测试查找不存在的设备
    std::cout << "测试查找不存在的设备..." << std::endl;
    auto nonexistent_equip = connections_->get_equipment_by_fd(999);
    auto nonexistent_by_id =
        connections_->get_equipment_by_id("nonexistent_device");
    int nonexistent_fd =
        connections_->get_fd_by_equipment_id("nonexistent_device");

    if (!nonexistent_equip && !nonexistent_by_id && nonexistent_fd == -1) {
      std::cout << "✅ 查找不存在设备处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 查找不存在设备处理测试失败" << std::endl;
    }
  }

  void run_all_tests() {
    test_database_operations();

    if (g_running) {
      test_connection_management();
    }

    if (g_running) {
      test_error_conditions();
    }

    // 清理
    if (connections_) {
      connections_->close_all_connections();
    }
  }
};

int main() {
  // 注册信号处理
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "开始 SimulatorConnections 测试..." << std::endl;

  SimulatorConnectionsTester tester;

  try {
    tester.run_all_tests();
    std::cout << "\n🎉 SimulatorConnections 测试完成!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "❌ 测试过程中发生异常: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "测试程序退出" << std::endl;
  return 0;
}