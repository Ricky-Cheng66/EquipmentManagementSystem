#pragma once

#include "connection_manager.h"
#include "database_manager.h"
#include "epoll.h"
#include "equipment_manager.h"
#include "message_buffer.h"
#include "protocol_parser.h"

#include <memory>
#include <string>
#include <unordered_map>

class EquipmentManagementServer {
public:
  EquipmentManagementServer()
      : equipment_manager_(std::make_unique<EquipmentManager>()),
        connections_manager_(std::make_unique<ConnectionManager>()),
        db_manager_(std::make_unique<DatabaseManager>()) {}

  bool init(int server_port);
  bool start();
  bool initialize_database();
  bool process_events(int nfds, struct epoll_event *evs);
  void handle_client_data(int fd);
  void process_single_message(int fd, const std::string &message);
  bool accept_new_connection();
  void handle_connection_close(int fd);
  void perform_maintenance_tasks();
  MessageBuffer *get_message_buffer(int fd);

  bool validate_user_exists(int user_id);
  bool validate_admin_permission(const std::string &admin_id);
  bool check_reservation_conflict(const std::string &equipment_id,
                                  const std::string &start_time,
                                  const std::string &end_time);
  // 远程控制接口
  bool send_control_command(const std::string &equipment_id,
                            const std::string &command);

  bool
  send_advanced_control_command(const std::string &equipment_id,
                                ProtocolParser::ControlCommandType command_type,
                                const std::string &parameters = "");

  bool
  send_batch_control_command(const std::vector<std::string> &equipment_ids,
                             ProtocolParser::ControlCommandType command_type,
                             const std::string &parameters = "");

  std::vector<std::string>
  get_equipment_control_capabilities(const std::string &equipment_id);

private:
  // 处理来自客户端的控制命令
  void handle_client_control_command(int fd, const std::string &equipment_id,
                                     const std::string &payload);
  //消息处理函数
  void handle_equipment_register(int fd, const std::string &device_id,
                                 const std::string &payload);
  void handle_status_update(int fd, const std::string &device_id,
                            const std::string &payload);
  void handle_control_command(int fd, const std::string &device_id,
                              const std::string &payload);
  void handle_heartbeat(int fd, const std::string &equipment_id);

  //预约处理函数
  void handle_reservation_apply(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_query(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_approve(int fd, const std::string &admin_id,
                                  const std::string &payload);
  void check_heartbeat_timeout();

  const int MAXCLIENTFDS = 1024;
  int server_fd_;
  std::string server_addr_;
  int server_port_;
  std::unique_ptr<EquipmentManager> equipment_manager_;
  std::unique_ptr<ConnectionManager> connections_manager_;
  std::unique_ptr<ProtocolParser> protocol_parser_;
  std::unique_ptr<DatabaseManager> db_manager_;
  std::unordered_map<int, std::unique_ptr<MessageBuffer>> message_buffers_;
};