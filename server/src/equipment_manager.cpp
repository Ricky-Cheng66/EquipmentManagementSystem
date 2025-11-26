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
        equipment_id,
        std::make_shared<Equipment>(equipment_id, equipment_type, location));
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

// 设备状态管理
bool EquipmentManager::update_equipment_status_from_simulator(
    const std::string &equipment_id, const std::string &status) {

  std::unique_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    it->second->update_status(status);
    std::cout << "设备状态更新: " << equipment_id << " -> " << status
              << std::endl;
    return true;
  }
  return false;
}

bool EquipmentManager::update_equipment_power_from_simulator(
    const std::string &equipment_id, const std::string &power_state) {

  std::unique_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    it->second->update_equipment_power_state(power_state);
    std::cout << "设备电源状态更新: " << equipment_id << " -> " << power_state
              << std::endl;
    return true;
  }
  return false;
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