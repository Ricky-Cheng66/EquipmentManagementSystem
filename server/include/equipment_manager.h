#pragma once
#include "equipment.h"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
class EquipmentManager {
public:
  // 设备生命周期管理
  bool register_equipment(const std::string &equipment_id,
                          const std::string &equipment_type,
                          const std::string &location);
  bool unregister_equipment(const std::string &equipment_id);

  // 设备状态管理
  bool update_equipment_status(const std::string &equipment_id,
                               const std::string &status);
  bool update_equipment_power_state(const std::string &equipment_id,
                                    const std::string &power_state);

  // 设备查询
  std::shared_ptr<Equipment> get_equipment(const std::string &equipment_id);
  std::vector<std::shared_ptr<Equipment>> get_all_equipments();
  size_t get_equipment_count() const;

  // 控制指令
  bool send_control_command(const std::string &equipment_id,
                            const std::string &command);

private:
  std::unordered_map<std::string, std::shared_ptr<Equipment>> equipments_;
  mutable std::shared_mutex equipment_rw_lock_;
  // DatabaseManager *m_db_manager; // 后续添加数据库管理
};