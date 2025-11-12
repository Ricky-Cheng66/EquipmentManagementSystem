#pragma once

#include "connection_manager.h"
#include "epoll.h"
#include "equipment_manager.h"
#include "protocol_parser.h"
#include <memory>
#include <string>
#include <unordered_map>

class EquipmentManagementServer {
public:
  EquipmentManagementServer()
      : equipment_manager_(std::make_unique<EquipmentManager>()),
        connections_manager_(std::make_unique<ConnectionManager>()) {}

  bool init(int server_port);
  bool start();
  bool process_events(int nfds, struct epoll_event *evs);
  void handle_client_data(int fd);
  bool accept_new_connection();
  void handle_connection_close(int fd);
  void perform_maintenance_tasks();

private:
  //消息处理函数
  void handle_equipment_register(int fd, const std::string &device_id,
                                 const std::string &payload);
  void handle_status_update(int fd, const std::string &device_id,
                            const std::string &payload);
  void handle_control_command(int fd, const std::string &device_id,
                              const std::string &payload);
  void handle_heartbeat(int fd, const std::string &equipment_id);
  void check_heartbeat_timeout();

  const int MAXCLIENTFDS = 1024;
  int server_fd_;
  std::string server_addr_;
  int server_port_;
  std::unique_ptr<EquipmentManager> equipment_manager_;
  std::unique_ptr<ConnectionManager> connections_manager_;
  std::unique_ptr<ProtocolParser> protocol_parser_;
};