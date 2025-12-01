#include "equipment_manager.h"
#include "protocol_parser.h"
#include <iostream>
// 设备生命周期管理
bool EquipmentManager::register_equipment(const std::string &equipment_id,
                                          const std::string &equipment_type,
                                          const std::string &location) {
  std::unique_lock lock(equipment_rw_lock_);
  auto it1 = equipments_.find(equipment_id);
  if (it1 != equipments_.end()) {
    std::cout << "this id is alread in equipments_..." << std::endl;
    return false;
  } else {
    auto [it2, inserted] = equipments_.emplace(
        equipment_id, std::make_shared<Equipment>(equipment_id, equipment_type,
                                                  location, "offline", "off"));
    if (inserted) {
      std::cout << "register sucess... equipment_id is" << equipment_id
                << std::endl;
      return true;
    } else {
      std::cout << "register failed..." << std::endl;
      return false;
    }
  }
}
bool EquipmentManager::unregister_equipment(const std::string &equipment_id) {
  std::unique_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    equipments_.erase(it);
    return true;
  } else {
    std::cout << "this equipment_id :" << equipment_id
              << "is not in equipments_...." << std::endl;
    return false;
  }
}

bool EquipmentManager::initialize_from_database(DatabaseManager *db_manager) {
  if (!db_manager || !db_manager->is_connected()) {
    std::cerr << "数据库未连接，无法初始化设备管理器" << std::endl;
    return false;
  }
  std::unique_lock lock(equipment_rw_lock_);
  equipments_.clear();

  // 从 equipments 表加载所有设备（因为 equipments 表只包含已注册设备）
  std::string query =
      "SELECT equipment_id, equipment_name, equipment_type, location, "
      "status, power_state FROM equipments";

  auto results = db_manager->execute_query(query);

  int loaded_count = 0;
  for (const auto &row : results) {
    if (row.size() >= 6) {
      std::string equipment_id = row[0];
      std::string equipment_name = row[1];
      std::string equipment_type = row[2];
      std::string location = row[3];
      std::string status = row[4];
      std::string power_state = row[5];

      // 创建设备对象
      auto equipment =
          std::make_shared<Equipment>(equipment_id, equipment_type, location);
      equipment->update_status(status);
      equipment->update_equipment_power_state(power_state);

      // 添加到管理器
      equipments_.emplace(equipment_id, equipment);
      loaded_count++;

      std::cout << "加载设备: " << equipment_id << " [" << equipment_type
                << "] 位置: " << location << " 状态: " << status
                << " 电源: " << power_state << std::endl;
    }
  }

  std::cout << "设备管理器初始化完成，加载 " << loaded_count << " 个已注册设备"
            << std::endl;
  return true;
}

// 设备状态管理
// 1. 添加 update_equipment_status 方法（只更新内存）
bool EquipmentManager::update_equipment_status(const std::string &equipment_id,
                                               const std::string &status) {
  std::unique_lock lock(equipment_rw_lock_);

  auto it = equipments_.find(equipment_id);
  if (it == equipments_.end()) {
    std::cout << "设备不存在，无法更新状态: " << equipment_id << std::endl;
    return false;
  }

  // 只更新内存中的设备状态
  it->second->update_status(status);

  std::cout << "设备状态更新成功（内存）: " << equipment_id << " -> " << status
            << std::endl;
  return true;
}

// 2. 添加 update_equipment_power_state 方法（只更新内存）
bool EquipmentManager::update_equipment_power_state(
    const std::string &equipment_id, const std::string &power_state) {

  std::unique_lock lock(equipment_rw_lock_);

  auto it = equipments_.find(equipment_id);
  if (it == equipments_.end()) {
    std::cout << "设备不存在，无法更新电源状态: " << equipment_id << std::endl;
    return false;
  }

  // 只更新内存中的设备电源状态
  it->second->update_equipment_power_state(power_state);

  std::cout << "设备电源状态更新成功（内存）: " << equipment_id << " -> "
            << power_state << std::endl;
  return true;
}

// 3. 添加 reset_all_equipment_status 方法（只重置内存状态）
void EquipmentManager::reset_all_equipment_status() {
  std::unique_lock lock(equipment_rw_lock_);

  for (auto &[equipment_id, equipment_ptr] : equipments_) {
    // 重置内存中的状态为离线且电源关闭
    equipment_ptr->update_status("offline");
    equipment_ptr->update_equipment_power_state("off");
    std::cout << "设备状态重置（内存）: " << equipment_id << std::endl;
  }
}

// 设备查询
std::shared_ptr<Equipment>
EquipmentManager::get_equipment(const std::string &equipment_id) {
  std::shared_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    return it->second;
  } else {
    std::cout << "this equipment_id :" << equipment_id
              << "is not in equipments_...." << std::endl;
    return nullptr;
  }
}
std::vector<std::shared_ptr<Equipment>> EquipmentManager::get_all_equipments() {
  std::shared_lock lock(equipment_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> all_equipments{};
  for (const auto &[k, v] : equipments_) {
    all_equipments.emplace_back(v);
  }
  return all_equipments;
}

size_t EquipmentManager::get_equipment_count() const {
  std::shared_lock lock(equipment_rw_lock_);
  return equipments_.size();
}

std::vector<std::string>
EquipmentManager::get_equipment_capabilities(const std::string &equipment_id) {

  auto equipment = get_equipment(equipment_id);
  if (!equipment) {
    return {};
  }

  std::vector<std::string> capabilities;
  std::string equipment_type = equipment->get_equipment_type();

  // 根据设备类型返回支持的控制能力
  if (equipment_type == "projector") {
    capabilities = {"turn_on", "turn_off", "restart", "adjust_brightness",
                    "input_source"};
  } else if (equipment_type == "air_conditioner") {
    capabilities = {"turn_on", "turn_off", "set_temperature", "set_mode",
                    "set_fan_speed"};
  } else if (equipment_type == "camera") {
    capabilities = {"turn_on", "turn_off", "start_recording", "stop_recording",
                    "adjust_angle"};
  } else {
    capabilities = {"turn_on", "turn_off", "restart"};
  }

  return capabilities;
}