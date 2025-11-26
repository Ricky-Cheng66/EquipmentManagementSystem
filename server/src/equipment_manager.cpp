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
  // 解析字符串命令（向后兼容）
  ProtocolParser::ControlCommandType cmd_type;
  std::string params;

  if (command == "turn_on") {
    cmd_type = ProtocolParser::TURN_ON;
  } else if (command == "turn_off") {
    cmd_type = ProtocolParser::TURN_OFF;
  } else if (command == "restart") {
    cmd_type = ProtocolParser::RESTART;
  } else {
    cmd_type = ProtocolParser::ADJUST_SETTINGS;
    params = command;
  }

  return send_control_command(equipment_id, cmd_type, params);
}

bool EquipmentManager::send_control_command(
    const std::string &equipment_id,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {
  std::unique_lock lock(equipment_rw_lock_);

  auto it = equipments_.find(equipment_id);
  if (it == equipments_.end()) {
    std::cout << "设备不存在: " << equipment_id << std::endl;
    return false;
  }

  return execute_control_command(equipment_id, command_type, parameters);
}

bool EquipmentManager::send_batch_control_commands(
    const std::vector<std::string> &equipment_ids,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {

  bool all_success = true;
  for (const auto &equipment_id : equipment_ids) {
    if (!send_control_command(equipment_id, command_type, parameters)) {
      all_success = false;
      std::cerr << "批量控制失败: " << equipment_id << std::endl;
    }
  }
  return all_success;
}

bool EquipmentManager::execute_control_command(
    const std::string &equipment_id,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {
  auto equipment = get_equipment(equipment_id);
  if (!equipment) {
    return false;
  }

  std::cout << "执行控制命令: " << equipment_id
            << " 类型: " << static_cast<int>(command_type)
            << " 参数: " << parameters << std::endl;

  switch (command_type) {
  case ProtocolParser::TURN_ON:
    equipment->update_equipment_power_state("on");
    equipment->update_status("online");
    break;

  case ProtocolParser::TURN_OFF:
    equipment->update_equipment_power_state("off");
    equipment->update_status("offline");
    break;

  case ProtocolParser::RESTART:
    // 模拟重启过程
    equipment->update_equipment_power_state("off");
    equipment->update_status("restarting");
    // 在实际系统中，这里会有延迟后重新上线的逻辑
    break;

  case ProtocolParser::ADJUST_SETTINGS:
    // 处理参数设置
    equipment->update_status("adjusting");
    std::cout << "调整设备设置: " << equipment_id << " 参数: " << parameters
              << std::endl;
    break;

  case ProtocolParser::GET_STATUS:
    // 触发状态更新
    std::cout << "设备状态: " << equipment->get_status()
              << " 电源: " << equipment->get_power_state() << std::endl;
    break;

  default:
    std::cerr << "未知控制命令类型: " << static_cast<int>(command_type)
              << std::endl;
    return false;
  }

  return true;
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