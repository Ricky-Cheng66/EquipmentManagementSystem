#pragma once

#include "equipment.h"
#include "protocol_parser.h"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
class ConnectionManager {
public:
  // 连接管理
  void add_connection(int fd, std::shared_ptr<Equipment> equipment);
  void remove_connection(int fd);
  void close_all_connections();

  // 设备查找
  std::shared_ptr<Equipment> get_equipment_by_fd(int fd);
  std::shared_ptr<Equipment>
  get_equipment_by_id(const std::string &Equipment_id);
  int get_fd_by_equipment_id(const std::string &equipment_id);
  std::vector<std::shared_ptr<Equipment>> get_all_equipments();

  // 状态管理
  void update_heartbeat(int fd);
  void check_heartbeat_timeout(int timeout_seconds = 60);
  bool is_connection_healthy(int fd) const;
  bool is_equipment_connection_healthy(const std::string &equipment_id) const;
  //连接状态查询
  bool is_equipment_connected(const std::string &equipment_id) const;
  bool is_connection_alive(int fd) const;

  //工具函数
  size_t get_connection_count() const;
  void print_connections() const;

  // 控制命令转发
  bool
  send_control_to_simulator(ProtocolParser::ClientType client_type,
                            const std::string &equipment_id,
                            ProtocolParser::ControlCommandType command_type,
                            const std::string &parameters = "");

  bool send_batch_control_to_simulator(
      ProtocolParser::ClientType client_type,
      const std::vector<std::string> &equipment_ids,
      ProtocolParser::ControlCommandType command_type,
      const std::string &parameters = "");

private:
  mutable std::shared_mutex connection_rw_lock_;
  std::unordered_map<int, std::shared_ptr<Equipment>>
      connections_;                                      // fd -> 设备
  std::unordered_map<int, time_t> heartbeat_times_;      // fd -> 心跳时间
  std::unordered_map<std::string, int> equipment_to_fd_; // 设备ID -> fd
  std::unordered_map<int, bool> connection_healthy_; // fd -> 连接健康状态
};