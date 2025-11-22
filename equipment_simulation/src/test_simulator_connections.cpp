// test_simulator_connections.cpp
#include "simulator_connections.h"
#include <cassert>
#include <iostream>
#include <memory>

class SimulatorConnectionsTester {
public:
  void run_all_tests() {
    std::cout << "=== 开始模拟器连接管理测试 ===" << std::endl;

    test_database_initialization();
    test_connection_management();
    test_equipment_lookup();
    test_status_management();
    test_batch_operations();
    test_error_cases();

    std::cout << "=== 所有测试完成 ===" << std::endl;
  }

private:
  void test_database_initialization() {
    std::cout << "\n--- 测试1: 数据库初始化 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();

    // 使用你的实际数据库连接信息
    bool success = simulator_conn->initialize_from_database(
        "localhost", "root", "509876.zxn", "equipment_management", 3306);

    if (success) {
      std::cout << "✅ 数据库初始化测试通过" << std::endl;

      // 验证加载的设备数量
      size_t total_count = simulator_conn->get_equipment_count();
      size_t registered_count = simulator_conn->get_registered_count();
      size_t pending_count = simulator_conn->get_pending_count();

      std::cout << "总设备数: " << total_count << std::endl;
      std::cout << "已注册设备: " << registered_count << std::endl;
      std::cout << "待注册设备: " << pending_count << std::endl;

      // 根据你的数据，应该有5个设备，其中2个registered，2个pending，1个unregistered
      if (total_count >= 4) { // 至少加载了registered和pending的设备
        std::cout << "✅ 设备数量验证通过" << std::endl;
      } else {
        std::cout << "❌ 设备数量验证失败" << std::endl;
      }

      simulator_conn->print_statistics();
      simulator_conn->print_all_equipments();
    } else {
      std::cout << "❌ 数据库初始化测试失败" << std::endl;
    }
  }

  void test_connection_management() {
    std::cout << "\n--- 测试2: 连接管理 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();
    if (!simulator_conn->initialize_from_database(
            "localhost", "root", "509876.zxn", "equipment_management")) {
      std::cout << "❌ 数据库初始化失败，跳过连接管理测试" << std::endl;
      return;
    }

    // 测试添加连接（使用已注册和待注册的设备）
    bool add1 =
        simulator_conn->add_connection(100, "real_proj_001"); // registered
    bool add2 =
        simulator_conn->add_connection(101, "real_ac_001"); // registered
    bool add3 = simulator_conn->add_connection(102, "real_proj_002"); // pending
    bool add4 =
        simulator_conn->add_connection(103, "real_camera_001"); // pending

    if (add1 && add2 && add3 && add4) {
      std::cout << "✅ 添加连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 添加连接测试失败" << std::endl;
    }

    std::cout << "当前连接数: " << simulator_conn->get_connection_count()
              << std::endl;
    simulator_conn->print_connections();

    // 测试重复添加
    bool duplicate = simulator_conn->add_connection(100, "real_proj_001");
    if (!duplicate) {
      std::cout << "✅ 重复连接检测测试通过" << std::endl;
    } else {
      std::cout << "❌ 重复连接检测测试失败" << std::endl;
    }

    // 测试连接状态检查
    if (simulator_conn->has_connection(100) &&
        simulator_conn->is_equipment_connected("real_proj_001")) {
      std::cout << "✅ 连接状态检查测试通过" << std::endl;
    } else {
      std::cout << "❌ 连接状态检查测试失败" << std::endl;
    }

    // 测试移除连接
    simulator_conn->remove_connection(101);
    if (!simulator_conn->has_connection(101) &&
        !simulator_conn->is_equipment_connected("real_ac_001")) {
      std::cout << "✅ 移除连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 移除连接测试失败" << std::endl;
    }

    // 测试按设备ID移除连接
    simulator_conn->remove_connection_by_equipment_id("real_proj_002");
    if (!simulator_conn->is_equipment_connected("real_proj_002")) {
      std::cout << "✅ 按设备ID移除连接测试通过" << std::endl;
    } else {
      std::cout << "❌ 按设备ID移除连接测试失败" << std::endl;
    }

    std::cout << "移除后连接数: " << simulator_conn->get_connection_count()
              << std::endl;
  }

  void test_equipment_lookup() {
    std::cout << "\n--- 测试3: 设备查找 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();
    if (!simulator_conn->initialize_from_database(
            "localhost", "root", "509876.zxn", "equipment_management")) {
      std::cout << "❌ 数据库初始化失败，跳过设备查找测试" << std::endl;
      return;
    }

    simulator_conn->add_connection(200, "real_proj_001");
    simulator_conn->add_connection(201, "real_ac_001");

    // 测试按FD查找设备
    auto equip1 = simulator_conn->get_equipment_by_fd(200);
    if (equip1 && equip1->get_equipment_id() == "real_proj_001") {
      std::cout << "✅ 按FD查找设备测试通过" << std::endl;
      std::cout << "  找到设备: " << equip1->get_equipment_id()
                << " 类型: " << equip1->get_equipment_type()
                << " 位置: " << equip1->get_location() << std::endl;
    } else {
      std::cout << "❌ 按FD查找设备测试失败" << std::endl;
    }

    // 测试按设备ID查找
    auto equip2 = simulator_conn->get_equipment_by_id("real_ac_001");
    if (equip2 && equip2->get_equipment_id() == "real_ac_001") {
      std::cout << "✅ 按设备ID查找测试通过" << std::endl;
    } else {
      std::cout << "❌ 按设备ID查找测试失败" << std::endl;
    }

    // 测试按设备ID查找FD
    int fd = simulator_conn->get_fd_by_equipment_id("real_proj_001");
    if (fd == 200) {
      std::cout << "✅ 按设备ID查找FD测试通过" << std::endl;
    } else {
      std::cout << "❌ 按设备ID查找FD测试失败" << std::endl;
    }

    // 测试获取设备列表
    auto all_equipments = simulator_conn->get_all_equipments();
    auto registered_equipments = simulator_conn->get_registered_equipments();
    auto pending_equipments = simulator_conn->get_pending_equipments();
    auto connected_equipments = simulator_conn->get_connected_equipments();

    std::cout << "所有设备数: " << all_equipments.size() << std::endl;
    std::cout << "已注册设备数: " << registered_equipments.size() << std::endl;
    std::cout << "待注册设备数: " << pending_equipments.size() << std::endl;
    std::cout << "已连接设备数: " << connected_equipments.size() << std::endl;

    if (!all_equipments.empty() && !registered_equipments.empty() &&
        !pending_equipments.empty()) {
      std::cout << "✅ 设备列表获取测试通过" << std::endl;
    } else {
      std::cout << "❌ 设备列表获取测试失败" << std::endl;
    }
  }

  void test_status_management() {
    std::cout << "\n--- 测试4: 状态管理 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();
    if (!simulator_conn->initialize_from_database(
            "localhost", "root", "509876.zxn", "equipment_management")) {
      std::cout << "❌ 数据库初始化失败，跳过状态管理测试" << std::endl;
      return;
    }

    // 测试单个设备状态更新
    bool status_updated =
        simulator_conn->update_equipment_status("real_proj_001", "online");
    bool power_updated =
        simulator_conn->update_equipment_power_state("real_proj_001", "on");

    if (status_updated && power_updated) {
      std::cout << "✅ 单个设备状态更新测试通过" << std::endl;

      // 验证状态更新
      auto equipment = simulator_conn->get_equipment_by_id("real_proj_001");
      if (equipment && equipment->get_status() == "online" &&
          equipment->get_power_state() == "on") {
        std::cout << "✅ 状态验证测试通过" << std::endl;
        std::cout << "  设备状态: " << equipment->get_status()
                  << " 电源状态: " << equipment->get_power_state() << std::endl;
      } else {
        std::cout << "❌ 状态验证测试失败" << std::endl;
      }
    } else {
      std::cout << "❌ 单个设备状态更新测试失败" << std::endl;
    }

    // 测试不存在的设备状态更新
    bool invalid_update =
        simulator_conn->update_equipment_status("nonexistent_device", "online");
    if (!invalid_update) {
      std::cout << "✅ 不存在的设备状态更新测试通过" << std::endl;
    } else {
      std::cout << "❌ 不存在的设备状态更新测试失败" << std::endl;
    }
  }

  void test_batch_operations() {
    std::cout << "\n--- 测试5: 批量操作 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();
    if (!simulator_conn->initialize_from_database(
            "localhost", "root", "509876.zxn", "equipment_management")) {
      std::cout << "❌ 数据库初始化失败，跳过批量操作测试" << std::endl;
      return;
    }

    // 测试批量状态更新
    simulator_conn->batch_update_status("offline");
    simulator_conn->batch_update_power_state("off");

    std::cout << "✅ 批量状态更新测试完成" << std::endl;

    // 验证批量更新结果
    auto all_equipments = simulator_conn->get_all_equipments();
    bool all_offline = true;
    bool all_off = true;

    for (const auto &equipment : all_equipments) {
      if (equipment->get_status() != "offline") {
        all_offline = false;
      }
      if (equipment->get_power_state() != "off") {
        all_off = false;
      }
    }

    if (all_offline && all_off) {
      std::cout << "✅ 批量状态验证测试通过" << std::endl;
    } else {
      std::cout << "❌ 批量状态验证测试失败" << std::endl;
    }

    simulator_conn->print_all_equipments();
  }

  void test_error_cases() {
    std::cout << "\n--- 测试6: 错误情况处理 ---" << std::endl;

    auto simulator_conn = std::make_unique<SimulatorConnections>();
    if (!simulator_conn->initialize_from_database(
            "localhost", "root", "509876.zxn", "equipment_management")) {
      std::cout << "❌ 数据库初始化失败，跳过错误情况测试" << std::endl;
      return;
    }

    // 测试无效FD
    bool invalid_fd = simulator_conn->add_connection(-1, "real_proj_001");
    if (!invalid_fd) {
      std::cout << "✅ 无效FD处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 无效FD处理测试失败" << std::endl;
    }

    // 测试不存在的设备连接
    bool nonexistent_equipment =
        simulator_conn->add_connection(300, "nonexistent_device");
    if (!nonexistent_equipment) {
      std::cout << "✅ 不存在设备连接处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 不存在设备连接处理测试失败" << std::endl;
    }

    // 测试移除不存在的连接
    simulator_conn->remove_connection(999); // 不存在的FD
    simulator_conn->remove_connection_by_equipment_id("nonexistent_device");
    std::cout << "✅ 移除不存在连接处理测试通过" << std::endl;

    // 测试查找不存在的设备
    auto nonexistent_equip = simulator_conn->get_equipment_by_fd(999);
    auto nonexistent_by_id =
        simulator_conn->get_equipment_by_id("nonexistent_device");
    int nonexistent_fd =
        simulator_conn->get_fd_by_equipment_id("nonexistent_device");

    if (!nonexistent_equip && !nonexistent_by_id && nonexistent_fd == -1) {
      std::cout << "✅ 查找不存在设备处理测试通过" << std::endl;
    } else {
      std::cout << "❌ 查找不存在设备处理测试失败" << std::endl;
    }
  }
};

int main() {
  SimulatorConnectionsTester tester;
  tester.run_all_tests();
  return 0;
}