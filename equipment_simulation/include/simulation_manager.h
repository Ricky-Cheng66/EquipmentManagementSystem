#pragma once

#include "epoll.h"
#include "message_buffer.h"
#include "protocol_parser.h"
#include "simulator_connections.h"
#include "socket.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

class SimulationManager {
public:
  SimulationManager();
  ~SimulationManager();

  // 删除拷贝构造和赋值操作
  SimulationManager(const SimulationManager &) = delete;
  SimulationManager &operator=(const SimulationManager &) = delete;

  // 初始化
  bool initialize(const std::string &server_ip, uint16_t server_port,
                  const std::string &db_host, const std::string &db_user,
                  const std::string &db_password,
                  const std::string &db_database, int db_port = 3306);

  // 启动和停止
  bool start();
  void stop();
  bool is_running() const { return is_running_; }

  // 设备控制接口
  bool connect_all_equipments();
  bool connect_equipment(const std::string &equipment_id);
  void disconnect_equipment(const std::string &equipment_id);
  void disconnect_all_equipments();

  // 状态查询
  void print_status() const;

private:
  // 事件循环处理
  void event_loop();
  bool process_events(int nfds, struct epoll_event *events);
  void handle_server_data(int fd);
  void process_single_message(int fd, const std::string &message);

  // 连接管理
  bool create_equipment_connection(const std::string &equipment_id);
  void handle_connection_close(int fd);

  // 消息处理
  void handle_register_response(int fd, const std::string &equipment_id,
                                bool success);
  void handle_heartbeat_response(int fd, const std::string &equipment_id);
  void handle_control_command(int fd, const std::string &equipment_id,
                              const std::string &command);

  // 定时任务
  void perform_maintenance_tasks();
  void send_heartbeats();
  void send_status_updates();
  void send_pending_registrations();

  // 消息发送
  bool send_message(int fd, const std::vector<char> &message);
  bool send_register_message(const std::string &equipment_id);
  bool send_heartbeat_message(const std::string &equipment_id);
  bool send_status_update_message(const std::string &equipment_id);

  // 工具函数
  MessageBuffer *get_message_buffer(int fd);
  bool add_to_epoll(int fd);

  // 成员变量
  std::unique_ptr<SimulatorConnections> connections_;

  std::string server_ip_;
  uint16_t server_port_;

  std::unordered_map<int, std::unique_ptr<MessageBuffer>> message_buffers_;

  std::atomic<bool> is_running_{false};
  std::thread event_loop_thread_;

  // 定时任务计数器
  int loop_count_{0};
  const int HEARTBEAT_INTERVAL = 5;      // 每5次循环发送一次心跳
  const int STATUS_UPDATE_INTERVAL = 10; // 每10次循环发送一次状态更新

  // 读写锁保护消息缓冲区
  mutable std::shared_mutex buffers_rw_lock_;
};