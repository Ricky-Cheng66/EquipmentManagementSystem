#include "equipment_manager.h"
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
bool EquipmentManager::update_equipment_status(const std::string &equipment_id,
                                               const std::string &status) {
  std::unique_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    it->second->update_status(status);
    return true;
  } else {
    std::cout << "this equipment_id :" << equipment_id
              << "is not in equipments_...." << std::endl;
    return false;
  }
}
bool EquipmentManager::update_equipment_power_state(
    const std::string &equipment_id, const std::string &power_state) {
  std::unique_lock lock(equipment_rw_lock_);
  auto it = equipments_.find(equipment_id);
  if (it != equipments_.end()) {
    it->second->update_equipment_power_state(power_state);
    return true;
  } else {
    std::cout << "this equipment_id :" << equipment_id
              << "is not in equipments_...." << std::endl;
    return false;
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

// 控制指令
bool EquipmentManager::send_control_command(const std::string &equipment_id,
                                            const std::string &command)

{
  return true;
}