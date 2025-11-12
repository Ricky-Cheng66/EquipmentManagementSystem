#pragma once

#include "equipment.h"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
class ConnectionManager {
public:
  // 连接管理
  void add_connection(int fd, std::shared_ptr<Equipment> Equipment);
  void remove_connection(int fd);
  std::shared_ptr<Equipment> get_equipment_by_fd(int fd);

  // 设备查找
  std::shared_ptr<Equipment>
  get_equipment_by_id(const std::string &Equipment_id);
  std::vector<std::shared_ptr<Equipment>> get_all_equipments();

  // 心跳管理
  void update_heartbeat(int fd);
  void check_heartbeat_timeout(int timeout_seconds = 60);

  //工具函数
  size_t get_connection_count() const;
  void print_connections() const;

private:
  std::unordered_map<int, std::shared_ptr<Equipment>>
      connections_; // fd -> 设备
  // 心跳时间映射：fd -> 最后心跳时间
  std::unordered_map<int, time_t> heartbeat_times_;
  mutable std::shared_mutex connection_rw_lock_;
};