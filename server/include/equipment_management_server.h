#pragma once

#include "connection_manager.h"
#include "database_manager.h"
#include "epoll.h"
#include "equipment_manager.h"
#include "message_buffer.h"
#include "protocol_parser.h"

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

class EquipmentManagementServer {
public:
  EquipmentManagementServer()
      : equipment_manager_(std::make_unique<EquipmentManager>()),
        connections_manager_(std::make_unique<ConnectionManager>()),
        db_manager_(std::make_unique<DatabaseManager>()) {}
  ~EquipmentManagementServer();
  //初始化
  bool init(int server_port);
  bool start();
  void stop();
  bool is_running() const { return is_running_; }
  void close_all_connections(); // 关闭所有客户端连接

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
  void process_equipment_message(int fd,
                                 const ProtocolParser::ParseResult &result);
  void process_qt_client_message(int fd,
                                 const ProtocolParser::ParseResult &result);

  // 处理Qt客户端登录
  void handle_qt_client_login(int fd, const std::string &equipment_id,
                              const std::string &payload);

  void handle_qt_equipment_List_query(int fd);

  // 具体消息类型处理
  void update_equipment_status_and_db(const std::string &equipment_id,
                                      const std::string &status,
                                      const std::string &power_state,
                                      const std::string &log_message = "");
  void handle_equipment_online(int fd, const std::string &equipment_id,
                               const std::string &payload);
  void handle_status_update(int fd, const std::string &equipment_id,
                            const std::string &payload);
  void handle_control_command_response_from_simulator(
      int fd, const std::string &equipment_id,
      const std::string &payload); //处理设备控制响应
  void handle_heartbeat(int fd, const std::string &equipment_id);

  //处理智能告警ack确认
  void handle_qt_alert_ack(int fd, const std::string &equipment_id,
                           const std::string &payload);

  // 处理Qt客户端心跳
  void handle_qt_heartbeat(int fd, const std::string &client_identifier);

  void handle_power_report(int fd, const std::string &equipment_id,
                           const std::string &payload);

  // 连接管理
  void handle_connection_close(int fd);
  void perform_maintenance_tasks();

  void reset_all_equipment_on_shutdown();

  //数据库
  bool initialize_database();

  // 消息缓冲区管理
  MessageBuffer *get_message_buffer(int fd);

  bool validate_user_exists(int user_id);
  bool validate_admin_permission(const std::string &admin_id);
  bool check_place_reservation_conflict(const std::string &equipment_id,
                                        const std::string &start_time,
                                        const std::string &end_time);
  // 远程控制接口
  bool send_control_command(const std::string &equipment_id,
                            ProtocolParser::ControlCommandType command_type,
                            const std::string &parameters = "");

  bool
  send_batch_control_command(const std::vector<std::string> &equipment_ids,
                             ProtocolParser::ControlCommandType command_type,
                             const std::string &parameters = "");

  bool send_control_command_response_to_qt_client(
      int qt_fd, const std::string &equipment_id, bool sucess,
      const std::string &reason);

  std::vector<std::string>
  get_equipment_control_capabilities(const std::string &equipment_id);

  //辅助函数
  std::string get_current_time();

private:
  // 处理来自客户端的控制命令
  void handle_qt_client_control_command(int fd, const std::string &equipment_id,
                                        const std::string &payload);
  //预约处理函数
  void handle_reservation_apply(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_query(int fd, const std::string &equipment_id,
                                const std::string &payload);
  void handle_reservation_approve(int fd, const std::string &admin_id,
                                  const std::string &payload);
  void check_heartbeat_timeout();

  void handle_qt_place_list_query(int fd);

  // 处理Qt客户端能耗查询请求
  void handle_qt_energy_query(int fd, const std::string &equipment_id,
                              const std::string &payload);

  void check_qt_client_heartbeat_timeout(int timeout_seconds);

  // 发送告警给所有Qt客户端
  void send_alert_to_all_qt_clients(const std::string &alarm_type,
                                    const std::string &equipment_id,
                                    const std::string &severity,
                                    const std::string &message);

  //成员变量
  const int MAXCLIENTFDS = 1024;
  int server_fd_;
  int server_port_;
  std::atomic<bool> is_running_{false}; // 添加运行状态标志
  std::thread server_thread_;           // 添加服务器线程
  std::unique_ptr<EquipmentManager> equipment_manager_;
  std::unique_ptr<ConnectionManager> connections_manager_;
  std::unique_ptr<DatabaseManager> db_manager_;
  std::unordered_map<int, std::unique_ptr<MessageBuffer>> message_buffers_;
};