#pragma once

#include "equipment.h"
#include "simulator_database_reader.h"
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class SimulatorConnections {
public:
  SimulatorConnections();
  ~SimulatorConnections() = default;

  // 删除拷贝构造和赋值操作
  SimulatorConnections(const SimulatorConnections &) = delete;
  SimulatorConnections &operator=(const SimulatorConnections &) = delete;

  // 初始化 - 从数据库读取设备信息（一次性操作）
  bool initialize_from_database(const std::string &host,
                                const std::string &user,
                                const std::string &password,
                                const std::string &database, int port = 3306);

  // 连接管理
  bool add_connection(int fd, const std::string &equipment_id);
  void remove_connection(int fd);
  void remove_connection_by_equipment_id(const std::string &equipment_id);
  bool has_connection(int fd) const;
  bool is_equipment_connected(const std::string &equipment_id) const;

  // 设备查找
  std::shared_ptr<Equipment> get_equipment_by_fd(int fd);
  std::shared_ptr<Equipment>
  get_equipment_by_id(const std::string &equipment_id);
  int get_fd_by_equipment_id(const std::string &equipment_id);

  // 设备列表获取
  std::vector<std::shared_ptr<Equipment>> get_all_equipments();
  std::vector<std::shared_ptr<Equipment>> get_registered_equipments();
  std::vector<std::shared_ptr<Equipment>> get_pending_equipments();
  std::vector<std::shared_ptr<Equipment>> get_connected_equipments();

  // 状态查询
  size_t get_connection_count() const;
  size_t get_equipment_count() const;
  size_t get_registered_count() const;
  size_t get_pending_count() const;

  // 设备状态管理（仅内存操作，不持久化到数据库）
  bool update_equipment_status(const std::string &equipment_id,
                               const std::string &status);
  bool update_equipment_power_state(const std::string &equipment_id,
                                    const std::string &power_state);
  void batch_update_status(const std::string &status);
  void batch_update_power_state(const std::string &power_state);

  // 工具函数
  void print_connections() const;
  void print_all_equipments() const;
  void print_statistics() const;

private:
  // 设备信息结构
  struct EquipmentInfo {
    std::shared_ptr<Equipment> equipment;
    std::string register_status; // "registered", "pending", "unregistered"
    std::string manufacturer;
    std::string model;
  };

  // 数据库读取器（只读）
  std::unique_ptr<SimulatorDatabaseReader> db_reader_;

  // 设备管理 - 所有真实设备（内存中）
  std::unordered_map<std::string, EquipmentInfo> real_equipments_;

  // 连接管理
  std::unordered_map<int, std::shared_ptr<Equipment>>
      fd_to_equipment_;                                  // fd -> Equipment
  std::unordered_map<std::string, int> equipment_to_fd_; // equipment_id -> fd

  // 读写锁保护共享数据
  mutable std::shared_mutex equipments_rw_lock_;
  mutable std::shared_mutex connections_rw_lock_;

  // 内部方法
  bool add_real_equipment(const std::string &equipment_id,
                          const std::string &equipment_name,
                          const std::string &equipment_type,
                          const std::string &location,
                          const std::string &register_status,
                          const std::string &manufacturer = "",
                          const std::string &model = "");
};