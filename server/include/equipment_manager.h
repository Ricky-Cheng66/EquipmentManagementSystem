#pragma once
#include "database_manager.h"
#include "equipment.h"
#include "protocol_parser.h"
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

  bool initialize_from_database(DatabaseManager *db_manager);

  // 状态查询（供Qt客户端使用）
  std::shared_ptr<Equipment> get_equipment(const std::string &equipment_id);
  std::vector<std::shared_ptr<Equipment>> get_all_equipments();
  size_t get_equipment_count() const;

  // 状态更新（仅用于从设备接收到的状态更新）
  bool update_equipment_status_from_simulator(const std::string &equipment_id,
                                              const std::string &status);
  bool update_equipment_power_from_simulator(const std::string &equipment_id,
                                             const std::string &power_state);
  // 控制能力查询（供Qt客户端使用）
  std::vector<std::string>
  get_equipment_capabilities(const std::string &equipment_id);
  // 设备状态管理
  bool update_equipment_status(const std::string &equipment_id,
                               const std::string &status);
  bool update_equipment_power_state(const std::string &equipment_id,
                                    const std::string &power_state);

private:
  mutable std::shared_mutex equipment_rw_lock_;
  std::unordered_map<std::string, std::shared_ptr<Equipment>> equipments_;
  // DatabaseManager *m_db_manager; // 后续添加数据库管理
};