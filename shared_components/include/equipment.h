#pragma once
#include <string>
class Equipment {
public:
  Equipment(const std::string &equipment_id, const std::string &equipment_type,
            const std::string &location, const std::string &status = "",
            const std::string &power_state = "")
      : equipment_id_(equipment_id), equipment_type_(equipment_type),
        location_(location) {}
  void update_status(const std::string &new_status);
  void update_equipment_power_state(const std::string &new_power_state);
  void update_heartbeat();
  std::string &get_equipment_id() { return equipment_id_; };
  std::string &get_equipment_type() { return equipment_type_; };
  std::string &get_location() { return location_; };
  std::string &get_status() { return status_; };
  std::string &get_power_state() { return power_state_; };
  time_t &get_last_heartbeat() { return last_heartbeat_; };
  bool is_online() const;

private:
  std::string equipment_id_;   //设备唯一标识
  std::string equipment_type_; //设备类型
  std::string equipment_name_; //设备名称
  std::string location_;       //设备位置
  std::string status_;         //在线状态:online,offline等
  std::string power_state_;    //电源状态
  time_t last_heartbeat_;      //最后心跳时间
};