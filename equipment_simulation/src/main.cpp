#include "equipment.h"
#include "equipment_stimulator.h"
#include <chrono>
#include <iostream>
#include <thread>

void test_basic_functionality() {
  std::cout << "=== 基础功能测试 ===" << std::endl;

  // 创建设备
  Equipment projector("projector_101", "projector", "classroom_101", "offline",
                      "off");

  // 创建模拟器（使用你的实际服务器地址）
  auto stimulator =
      EquipmentStimulator::create(projector, "192.168.198.129", 9000);

  if (!stimulator) {
    std::cerr << "❌ 模拟器创建失败" << std::endl;
    return;
  }

  std::cout << "✅ 模拟器创建成功: " << stimulator->get_equipment_id()
            << std::endl;

  // 测试注册功能
  std::cout << "测试设备注册..." << std::endl;
  stimulator->send_registration();

  // 等待服务器响应
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // 测试心跳
  std::cout << "测试心跳..." << std::endl;
  stimulator->send_heartbeat();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 测试状态更新
  std::cout << "测试状态更新..." << std::endl;
  stimulator->send_status_update();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "✅ 基础功能测试完成" << std::endl;
}

void test_multiple_devices() {
  std::cout << "\n=== 多设备测试 ===" << std::endl;

  // 创建多个不同类型的设备
  Equipment devices[] = {
      Equipment("projector_101", "projector", "classroom_101"),
      Equipment("camera_201", "camera", "lab_301"),
      Equipment("computer_301", "computer", "office_201")};

  for (auto &device : devices) {
    auto stimulator =
        EquipmentStimulator::create(device, "192.168.1.100", 9000);
    if (stimulator) {
      std::cout << "✅ 设备 " << device.get_equipment_id() << " 创建成功"
                << std::endl;
      stimulator->send_registration();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  std::cout << "✅ 多设备测试完成" << std::endl;
}

int main() {
  std::cout << "开始设备模拟器测试..." << std::endl;

  test_basic_functionality();
  test_multiple_devices();

  std::cout << "\n🎉 所有测试完成！" << std::endl;
  std::cout << "请检查服务器日志确认消息接收情况" << std::endl;

  return 0;
}