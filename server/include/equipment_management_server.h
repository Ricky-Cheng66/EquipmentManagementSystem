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
  //初始化
  bool init(int server_port);
  bool start();

  // Qt客户端接口
  bool
  handle_qt_control_request(const std::string &equipment_id,
                            ProtocolParser::ControlCommandType command_type,
                            const std::string &parameters = "");
  std::shared_ptr<Equipment>
  handle_qt_status_query(const std::string &equipment_id);

private:
  // 网络事件处理
  bool process_events(int nfds, struct epoll_event *evs);
  void handle_client_data(int fd);
  bool accept_new_connection();

  // 消息处理
  void process_single_message(int fd, const std::string &message);

  // 具体消息类型处理
  void handle_equipment_register(int fd, const std::string &equipment_id,
                                 const std::string &payload);
  void handle_status_update(int fd, const std::string &equipment_id,
                            const std::string &payload);
  void
  handle_control_response(int fd, const std::string &equipment_id,
                          const std::string &payload); // 新增：处理设备控制响应
  void handle_heartbeat(int fd, const std::string &equipment_id);

  // 连接管理
  void handle_connection_close(int fd);
  void perform_maintenance_tasks();

  //数据库
  bool initialize_database();

  // 消息缓冲区管理
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
  void handle_control_command(int fd, const std::string &equipment_id,
                              const std::string &payload);
  //预约处理函数
  void handle_reservation_apply(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_query(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_approve(int fd, const std::string &admin_id,
                                  const std::string &payload);
  void check_heartbeat_timeout();

  //成员变量
  const int MAXCLIENTFDS = 1024;
  int server_fd_;
  int server_port_;
  std::unique_ptr<EquipmentManager> equipment_manager_;
  std::unique_ptr<ConnectionManager> connections_manager_;
  std::unique_ptr<DatabaseManager> db_manager_;
  std::unordered_map<int, std::unique_ptr<MessageBuffer>> message_buffers_;
};