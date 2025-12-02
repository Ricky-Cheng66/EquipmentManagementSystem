#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <string.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

#include "connection_manager.h"
#include "epoll.h"
#include "equipment_management_server.h"
#include "protocol_parser.h"
#include "socket.h"

EquipmentManagementServer::~EquipmentManagementServer() { stop(); }
bool EquipmentManagementServer::init(int server_port) {
  // socket部分
  server_port_ = server_port;
  // create server_fd
  Socket server_socket{};
  server_fd_ = server_socket.create_socket();
  //设置地址重用
  if (!server_socket.set_socket_option(server_fd_)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "set_socket_option failed..." << ec.message() << std::endl;
  }
  // set listenFd nonblock
  server_socket.set_nonblock(server_fd_);
  // blind
  server_socket.bind_server_socket(server_fd_, server_port_);
  // listen
  server_socket.listen_socket(server_fd_);
  // create epfd and put listenFd into epoll
  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    return false;
  }
  // add_epoll
  ep.add_epoll(server_fd_, EPOLLIN | EPOLLET);
  return true;
}

bool EquipmentManagementServer::start() {
  if (is_running_) {
    std::cout << "服务器已经在运行" << std::endl;
    return true;
  }

  // 初始化数据库
  if (!initialize_database()) {
    std::cerr << "数据库初始化失败，服务器启动中止" << std::endl;
    return false;
  }
  // 启动服务器线程
  is_running_ = true;
  server_thread_ = std::thread([this]() {
    // 获取Epoll单例
    Epoll &ep = Epoll::get_instance();
    if (!ep.initialize()) {
      std::cerr << "Epoll初始化失败" << std::endl;
      return;
    }

    int max_events = ep.get_epoll_max_events();
    struct epoll_event *evs = new epoll_event[max_events]{};
    std::cout << "设备管理服务器启动成功，开始事件循环..." << std::endl;

    while (is_running_) {
      int nfds = ep.wait_events(evs, 100); // 100ms超时

      if (nfds < 0) {
        if (errno == EINTR && is_running_) {
          continue;
        }
        std::cerr << "epoll_wait错误: " << std::endl;
        break;
      } else if (nfds == 0) {
        // 超时，检查运行状态
        continue;
      }

      // 处理事件
      if (!process_events(nfds, evs)) {
        std::cerr << "事件处理失败..." << std::endl;
        break;
      }

      // 定期执行维护任务
      static int loop_count = 0;
      if (++loop_count >= 10) {
        perform_maintenance_tasks();
        loop_count = 0;
      }
    }

    delete[] evs;
    std::cout << "服务器线程退出" << std::endl;
  });

  std::cout << "服务器启动成功" << std::endl;
  return true;
}

void EquipmentManagementServer::stop() {
  if (!is_running_) {
    return;
  }

  std::cout << "正在停止服务器..." << std::endl;
  is_running_ = false;

  // 等待服务器线程结束
  if (server_thread_.joinable()) {
    server_thread_.join();
    std::cout << "服务器线程已停止" << std::endl;
  }

  // 重置所有设备状态
  reset_all_equipment_on_shutdown();

  // 关闭服务器socket
  if (server_fd_ > 0) {
    close(server_fd_);
    server_fd_ = -1;
  }

  std::cout << "服务器已完全停止" << std::endl;
}

void EquipmentManagementServer::close_all_connections() {
  connections_manager_->close_all_connections();
}

bool EquipmentManagementServer::initialize_database() {
  // 数据库连接配置
  std::string host = "localhost";
  std::string user = "root";
  std::string password = "509876.zxn"; // 修改为你的MySQL密码
  std::string database = "equipment_management";

  if (!db_manager_->connect(host, user, password, database)) {
    std::cerr << "数据库连接失败!" << std::endl;
    return false;
  }

  std::cout << "数据库连接成功!" << std::endl;

  // 从数据库初始化设备管理器（从 equipments 表）
  if (!equipment_manager_->initialize_from_database(db_manager_.get())) {
    std::cerr << "设备管理器初始化失败!" << std::endl;
    return false;
  }
  std::cout << "设备管理器初始化成功，共 "
            << equipment_manager_->get_equipment_count() << " 个已注册设备"
            << std::endl;
  return true;
}

MessageBuffer *EquipmentManagementServer::get_message_buffer(int fd) {
  auto it = message_buffers_.find(fd);
  if (it == message_buffers_.end()) {
    //为新连接创建缓冲区
    auto buffer = std::make_unique<MessageBuffer>();
    auto result = message_buffers_.emplace(fd, std::move(buffer));
    return result.first->second.get();
  }
  return it->second.get();
}

bool EquipmentManagementServer::process_events(int nfds,
                                               struct epoll_event *evs) {
  for (int i = 0; i < nfds; i++) {
    int event_fd = evs[i].data.fd;
    uint32_t events = evs[i].events;
    // 记录调试信息
    std::cout << "处理事件: fd=" << event_fd << ", events=0x" << std::hex
              << events << std::dec << std::endl;
    //检查错误事件
    if (events & (EPOLLERR | EPOLLHUP)) {
      std::cerr << "连接错误或挂起,关闭fd: " << event_fd << std::endl;
      handle_connection_close(event_fd);
      continue;
    }
    //服务器接受新连接
    if (event_fd == server_fd_) {
      if (!accept_new_connection()) {
        std::cerr << "接受新连接失败" << std::endl;
      }

    } else if (events & EPOLLIN) {
      handle_client_data(event_fd);
    } else {
      std::cout << "未处理的事件类型: 0x" << std::hex << events << std::dec
                << std::endl;
    }
  }
  return true;
}

void EquipmentManagementServer::handle_client_data(int fd) {
  char recv_buffer[2048]; // 系统级接收缓冲区

  // 一直收：能读多少读多少
  while (true) {
    int bytes_received = recv(fd, recv_buffer, sizeof(recv_buffer), 0);

    if (bytes_received > 0) {
      // 追加到应用层缓冲区
      MessageBuffer *msg_buffer = get_message_buffer(fd);
      msg_buffer->append_data(recv_buffer, bytes_received);

      // 循环解析：尝试提取所有完整消息
      std::vector<std::string> complete_messages;
      size_t extracted_count = msg_buffer->extract_messages(complete_messages);

      // 处理所有完整消息
      for (const auto &message : complete_messages) {
        process_single_message(fd, message);
      }

      // 检查缓冲区是否异常
      if (msg_buffer->is_too_large()) {
        std::cerr << "连接 " << fd << " 缓冲区异常，关闭连接" << std::endl;
        handle_connection_close(fd);
        return;
      }

    } else if (bytes_received == 0) {
      // 连接正常关闭
      std::cout << "客户端主动关闭连接: fd=" << fd << std::endl;
      handle_connection_close(fd);
      return;
    } else {
      // 错误处理
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 非阻塞模式下没有更多数据可读，正常退出循环
        break;
      } else {
        // 真正的错误
        std::cerr << "接收数据错误: fd=" << fd << std::endl;
        handle_connection_close(fd);
        return;
      }
    }
  }
}

void EquipmentManagementServer::process_single_message(
    int fd, const std::string &message) {
  // 使用你原有的消息解析逻辑
  auto parse_result = ProtocolParser::parse_message(message);

  if (!parse_result.success) {
    std::cout << "协议解析失败: " << message << std::endl;
    return;
  }

  // 根据消息类型分发处理
  switch (parse_result.type) {
  case ProtocolParser::EQUIPMENT_ONLINE:
    handle_equipment_online(fd, parse_result.equipment_id,
                            parse_result.payload);
    break;
  case ProtocolParser::STATUS_UPDATE:
    handle_status_update(fd, parse_result.equipment_id, parse_result.payload);
    break;
  case ProtocolParser::CONTROL_RESPONSE:
    handle_control_response(fd, parse_result.equipment_id,
                            parse_result.payload);
    break;
  case ProtocolParser::HEARTBEAT:
    handle_heartbeat(fd, parse_result.equipment_id);
    break;
  case ProtocolParser::RESERVATION_APPLY:
    handle_reservation_apply(fd, parse_result.equipment_id,
                             parse_result.payload);
    break;
  case ProtocolParser::RESERVATION_QUERY:
    handle_reservation_query(fd, parse_result.equipment_id,
                             parse_result.payload);
    break;
  case ProtocolParser::RESERVATION_APPROVE:
    handle_reservation_approve(fd, parse_result.equipment_id,
                               parse_result.payload);
    break;
  default:
    std::cout << "未知消息类型: " << parse_result.type << " from fd=" << fd
              << std::endl;
  }
}

// 新增：处理设备控制响应
void EquipmentManagementServer::handle_control_response(
    int fd, const std::string &equipment_id, const std::string &payload) {
  std::cout << "收到设备控制响应: " << equipment_id << " -> " << payload
            << std::endl;

  // 解析响应 (格式: "success|turn_on" 或 "fail|turn_off|reason")
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "控制响应格式错误: " << payload << std::endl;
    return;
  }

  bool success = (parts[0] == "success");
  std::string command = parts[1];

  if (success) {
    // 控制成功，只更新设备电源状态，不改变在线状态
    if (command == "turn_on") {
      equipment_manager_->update_equipment_power_state(equipment_id, "on");
    } else if (command == "turn_off") {
      equipment_manager_->update_equipment_power_state(equipment_id, "off");
    } else if (command == "restart") {
      equipment_manager_->update_equipment_power_state(equipment_id, "on");
    }

    // 记录到数据库
    if (db_manager_->is_connected()) {
      auto equipment = equipment_manager_->get_equipment(equipment_id);
      if (equipment) {
        db_manager_->update_equipment_status(equipment_id,
                                             equipment->get_status(),
                                             equipment->get_power_state());

        // 记录控制日志
        db_manager_->log_equipment_status(equipment_id, equipment->get_status(),
                                          equipment->get_power_state(),
                                          "control_success:" + command);
      }
    }

    std::cout << "控制命令执行成功: " << equipment_id << " -> " << command
              << std::endl;
  } else {
    std::cout << "控制命令执行失败: " << equipment_id << " -> " << command
              << std::endl;
    if (parts.size() > 2) {
      std::cout << "失败原因: " << parts[2] << std::endl;
    }
  }

  // 这里可以添加通知Qt客户端的逻辑
  // notify_qt_control_result(equipment_id, success, command);
}

// Qt客户端接口实现
bool EquipmentManagementServer::handle_qt_control_request(
    const std::string &equipment_id,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {

  std::cout << "处理Qt控制请求: " << equipment_id
            << " 命令: " << static_cast<int>(command_type)
            << " 参数: " << parameters << std::endl;

  // 通过ConnectionManager转发给设备
  return connections_manager_->send_control_to_simulator(
      equipment_id, command_type, parameters);
}

std::shared_ptr<Equipment> EquipmentManagementServer::handle_qt_status_query(
    const std::string &equipment_id) {

  return equipment_manager_->get_equipment(equipment_id);
}

void EquipmentManagementServer::update_equipment_status_and_db(
    const std::string &equipment_id, const std::string &status,
    const std::string &power_state, const std::string &log_message) {
  // 更新内存
  bool mem_status =
      equipment_manager_->update_equipment_status(equipment_id, status);
  bool mem_power = equipment_manager_->update_equipment_power_state(
      equipment_id, power_state);

  if (!mem_status || !mem_power) {
    std::cerr << "内存状态更新失败: " << equipment_id << std::endl;
    return;
  }

  // 更新数据库
  if (db_manager_->is_connected()) {
    bool db_success =
        db_manager_->update_equipment_status(equipment_id, status, power_state);

    if (db_success && !log_message.empty()) {
      db_manager_->log_equipment_status(equipment_id, status, power_state,
                                        log_message);
    }
  }
}

//处理设备注册
void EquipmentManagementServer::handle_equipment_online(
    int fd, const std::string &equipment_id, const std::string &payload) {

  std::cout << "DEBUG: 进入handle_equipment_register" << std::endl;

  // 检查成员变量是否初始化
  if (!equipment_manager_) {
    std::cerr << "ERROR: equipment_manager_ 未初始化!" << std::endl;
    return;
  }

  if (!connections_manager_) {
    std::cerr << "ERROR: connection_manager_ 未初始化!" << std::endl;
    return;
  }

  std::cout << "DEBUG: 开始解析payload: " << payload << std::endl;

  // 解析payload获取设备信息 (格式: "classroom_101|projector")
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "设备注册数据格式错误" << std::endl;
    return;
  }
  std::string location = parts[0];
  std::string equipment_type = parts[1];

  // 检查设备是否在EquipmentManager中（是否已注册）
  auto equipment = equipment_manager_->get_equipment(equipment_id);
  if (!equipment) {
    std::cout << "设备未注册，拒绝上线: " << equipment_id << std::endl;
    // 发送上线失败响应
    std::vector<char> response = ProtocolParser::build_online_response(false);
    send(fd, response.data(), response.size(), 0);
    return;
  }

  // 设备已注册，添加到连接管理
  connections_manager_->add_connection(fd, equipment);

  // 更新设备状态为在线（但不改变电源状态）
  equipment_manager_->update_equipment_status(equipment_id, "online");

  // 更新数据库中的设备状态
  if (db_manager_->is_connected()) {
    bool update_success = db_manager_->update_equipment_status(
        equipment_id, "online",
        equipment->get_power_state()); // 保持原有电源状态
    if (update_success) {
      // 修复：使用普通字符串而不是JSON
      bool log_success = db_manager_->log_equipment_status(
          equipment_id, "online", equipment->get_power_state(), "设备上线成功");

      if (!log_success) {
        std::cerr << "设备状态日志记录失败: " << equipment_id << std::endl;
        // 注意：不返回，继续执行
      }

      std::cout << "设备状态更新到数据库成功: " << equipment_id << std::endl;
    } else {
      std::cerr << "设备状态更新到数据库失败: " << equipment_id << std::endl;
      // 注意：不返回，继续执行
    }
  }

  // 发送上线成功响应
  std::vector<char> response = ProtocolParser::build_online_response(true);
  ssize_t bytes_sent = send(fd, response.data(), response.size(), 0);

  if (bytes_sent <= 0) {
    std::cerr << "上线响应发送失败: " << equipment_id << std::endl;
  } else {
    std::cout << "上线响应已发送: " << equipment_id << std::endl;
  }

  std::cout << "设备上线处理完成: " << equipment_id << std::endl;
}

//处理状态更新
void EquipmentManagementServer::handle_status_update(
    int fd, const std::string &equipment_id, const std::string &payload) {
  std::cout << "处理状态更新: " << equipment_id << " payload: " << payload
            << std::endl;
  //解析状态数据(格式: "online|on|45")
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "状态数据格式错误" << std::endl;
    return;
  }

  std::string status = parts[0];
  std::string power_state = parts[1];
  std::string additional_data = "";

  // 如果有额外数据（如温度）
  if (parts.size() > 2) {
    additional_data = parts[2];
  }

  update_equipment_status_and_db(equipment_id, status, power_state,
                                 "设备主动上报状态");

  //更新心跳时间
  connections_manager_->update_heartbeat(fd);
  std::cout << "设备状态更新: " << equipment_id << " -> " << status << ","
            << power_state << std::endl;
}

//处理控制指令(从设备端发来的控制响应)
void EquipmentManagementServer::handle_control_command(
    int fd, const std::string &equipment_id, const std::string &payload) {
  std::cout << "处理控制响应: " << equipment_id << " payload: " << payload
            << std::endl;

  // 这里处理设备对控制指令的响应
  // 比如设备执行turn_on后的确认消息

  //更新设备状态
  if (payload == "turn_on_success") {
    equipment_manager_->update_equipment_power_state(equipment_id, "on");
  } else if (payload == "turn_off_success") {
    equipment_manager_->update_equipment_power_state(equipment_id, "off");
  }
  std::cout << "控制响应处理完成: " << equipment_id << " -> " << payload
            << std::endl;
}

// 处理客户端控制命令
void EquipmentManagementServer::handle_client_control_command(
    int fd, const std::string &equipment_id, const std::string &payload) {

  std::cout << "处理控制命令: " << equipment_id << " payload: " << payload
            << std::endl;

  // 解析控制命令（格式: "command_type|parameters"）
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 1) {
    std::cout << "控制命令格式错误" << std::endl;
    return;
  }

  int command_type_num;
  try {
    command_type_num = std::stoi(parts[0]);
  } catch (const std::exception &e) {
    std::cout << "控制命令类型解析错误: " << parts[0] << std::endl;
    return;
  }

  ProtocolParser::ControlCommandType command_type =
      static_cast<ProtocolParser::ControlCommandType>(command_type_num);
  std::string parameters = parts.size() > 1 ? parts[1] : "";

  // 执行控制命令
  bool success = connections_manager_->send_control_to_simulator(
      equipment_id, command_type, parameters);

  // 发送控制响应
  std::string response_msg = success ? "命令执行成功" : "命令执行失败";
  std::vector<char> response = ProtocolParser::build_control_response(
      equipment_id, success, response_msg);

  send(fd, response.data(), response.size(), 0);

  // 记录到数据库
  if (db_manager_->is_connected()) {
    // 这里可以添加控制命令日志记录
    std::cout << "控制命令已记录: " << equipment_id << " -> "
              << command_type_num << std::endl;
  }
}

// 处理心跳
void EquipmentManagementServer::handle_heartbeat(
    int fd, const std::string &equipment_id) {
  //更新心跳时间
  connections_manager_->update_heartbeat(fd);

  // 发送心跳响应
  std::vector<char> response = ProtocolParser::build_heartbeat_response();
  ssize_t bytes_sent = send(fd, response.data(), response.size(), 0);

  if (bytes_sent > 0) {
    std::cout << "心跳处理: " << equipment_id << " fd=" << fd << " (响应已发送)"
              << std::endl;
  } else {
    std::cout << "心跳处理: " << equipment_id << " fd=" << fd
              << " (响应发送失败)" << std::endl;
  }
}

void EquipmentManagementServer::check_heartbeat_timeout() {
  connections_manager_->check_heartbeat_timeout(60); // 60秒超时
}

bool EquipmentManagementServer::accept_new_connection() {
  int accepted_count = 0;
  bool has_error = false;

  // ET模式下必须循环accept，直到没有更多连接
  while (true) {
    Socket server_socket{};
    int client_fd = server_socket.accept_socket(server_fd_);

    if (client_fd < 0) {
      // 检查是否没有更多连接了
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有更多连接可接受，正常退出
        if (accepted_count > 0) {
          std::cout << "本次循环接受 " << accepted_count << " 个新连接"
                    << std::endl;
        }
        break;
      }

      // 真正的错误
      std::cerr << "accept失败: " << strerror(errno) << std::endl;
      has_error = true;
      break;
    }

    accepted_count++;

    // 设置非阻塞
    Socket client_socket{};
    if (!client_socket.set_nonblock(client_fd)) {
      std::cerr << "设置非阻塞失败: " << client_fd << std::endl;
      close(client_fd);
      continue; // 继续接受其他连接
    }

    // 注册到epoll（ET模式）
    Epoll &ep = Epoll::get_instance();
    if (!ep.add_epoll(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP)) {
      std::cerr << "epoll注册失败: " << client_fd << std::endl;
      close(client_fd);
      continue;
    }

    // 获取客户端信息
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    if (getpeername(client_fd, (struct sockaddr *)&client_addr, &client_len) ==
        0) {
      std::cout << "新客户端连接[" << accepted_count << "]: fd=" << client_fd
                << ", IP=" << inet_ntoa(client_addr.sin_addr)
                << ", Port=" << ntohs(client_addr.sin_port) << std::endl;
    } else {
      std::cout << "新客户端连接[" << accepted_count << "]: fd=" << client_fd
                << " (无法获取地址)" << std::endl;
    }
  }

  return !has_error; // 有错误返回false，无错误返回true
}

void EquipmentManagementServer::handle_connection_close(int fd) {

  // 先检查文件描述符是否有效
  if (fd <= 0) {
    std::cout << "无效的文件描述符: " << fd << std::endl;
    return;
  }

  // 清理消息缓冲区
  message_buffers_.erase(fd);

  // 检查是否为设备连接
  auto equipment = connections_manager_->get_equipment_by_fd(fd);
  if (equipment) {
    std::cout << "设备离线: " << equipment->get_equipment_id() << std::endl;
    // 更新设备状态为离线
    equipment_manager_->update_equipment_status(equipment->get_equipment_id(),
                                                "offline");

    // 更新数据库
    if (db_manager_->is_connected()) {
      auto eq =
          equipment_manager_->get_equipment(equipment->get_equipment_id());
      if (eq) {
        db_manager_->update_equipment_status(equipment->get_equipment_id(),
                                             "offline", eq->get_power_state());
        db_manager_->log_equipment_status(equipment->get_equipment_id(),
                                          "offline", eq->get_power_state(),
                                          "连接关闭");
      }
    }
  } else {
    std::cout << "普通客户端连接关闭: fd=" << fd << std::endl;
  }

  // 从ConnectionManager中移除（如果存在）
  // 注意：remove_connection会自己检查连接是否存在
  connections_manager_->remove_connection(fd);

  // 从epoll中移除
  Epoll &ep = Epoll::get_instance();
  if (ep.is_initialized()) {
    ep.delete_epoll(fd);
  }

  // 关闭文件描述符
  if (fd > 0) {
    close(fd);
  }
  std::cout << "连接完全清理: fd=" << fd << std::endl;
}

void EquipmentManagementServer::perform_maintenance_tasks() {
  // 检查心跳超时
  connections_manager_->check_heartbeat_timeout(60);

  // 打印当前状态
  std::cout << "=== 系统状态 ===" << std::endl;
  std::cout << "活跃连接: " << connections_manager_->get_connection_count()
            << std::endl;
  std::cout << "注册设备: " << equipment_manager_->get_equipment_count()
            << std::endl;

  // 可选：打印详细连接信息
  connections_manager_->print_connections();
  std::cout << "=================" << std::endl;
}

// 2. 添加服务器停止时的状态重置方法
void EquipmentManagementServer::reset_all_equipment_on_shutdown() {
  std::cout << "服务器停止，重置所有设备状态..." << std::endl;

  // 1. 重置内存中的所有设备状态
  equipment_manager_->reset_all_equipment_status();

  // 2. 更新数据库中所有设备状态为离线且电源关闭
  if (db_manager_->is_connected()) {
    // 获取所有设备ID
    auto all_equipments = equipment_manager_->get_all_equipments();

    for (const auto &equipment : all_equipments) {
      bool success = db_manager_->update_equipment_status(
          equipment->get_equipment_id(), "offline", "off");

      if (success) {
        std::cout << "数据库状态重置成功: " << equipment->get_equipment_id()
                  << std::endl;
      } else {
        std::cerr << "数据库状态重置失败: " << equipment->get_equipment_id()
                  << std::endl;
      }
    }
  }

  // 3. 关闭所有连接
  close_all_connections();
  std::cout << "所有设备状态重置完成" << std::endl;
}

void EquipmentManagementServer::handle_reservation_apply(
    int fd, const std::string &equipment_id, const std::string &payload) {
  std::cout << "处理预约申请: " << equipment_id << " payload: " << payload
            << std::endl;

  // 解析payload格式: "user_id|start_time|end_time|purpose"
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 4) {
    std::cout << "预约申请数据格式错误" << std::endl;
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "数据格式错误");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  std::string user_id_str = parts[0];
  std::string start_time = parts[1];
  std::string end_time = parts[2];
  std::string purpose = parts[3];

  // 验证用户是否存在
  int user_id = std::stoi(user_id_str);
  if (!validate_user_exists(user_id)) {
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "用户不存在");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  // 检查设备是否存在
  auto equipment = equipment_manager_->get_equipment(equipment_id);
  if (!equipment) {
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "设备不存在");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  // 检查时间冲突
  if (check_reservation_conflict(equipment_id, start_time, end_time)) {
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "时间冲突");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  // 保存到数据库
  if (db_manager_->is_connected()) {
    bool success = db_manager_->add_reservation(equipment_id, user_id, purpose,
                                                start_time, end_time);
    if (success) {
      std::vector<char> response = ProtocolParser::build_reservation_response(
          true, "预约申请提交成功，等待审批");
      send(fd, response.data(), response.size(), 0);
      std::cout << "预约申请成功: " << equipment_id << " by user " << user_id
                << std::endl;
    } else {
      std::vector<char> response =
          ProtocolParser::build_reservation_response(false, "数据库错误");
      send(fd, response.data(), response.size(), 0);
    }
  } else {
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "系统错误");
    send(fd, response.data(), response.size(), 0);
  }
}

void EquipmentManagementServer::handle_reservation_query(
    int fd, const std::string &equipment_id, const std::string &payload) {
  std::cout << "处理预约查询: " << equipment_id << " payload: " << payload
            << std::endl;

  if (!db_manager_->is_connected()) {
    std::vector<char> response =
        ProtocolParser::build_reservation_response(false, "数据库连接失败");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  std::vector<std::vector<std::string>> reservations;

  if (equipment_id == "all") {
    // 查询所有预约
    reservations = db_manager_->get_all_reservations();
  } else {
    // 查询指定设备的预约
    reservations = db_manager_->get_reservations_by_equipment(equipment_id);
  }

  // 构建响应数据
  std::string response_data;
  for (const auto &reservation : reservations) {
    // 格式:
    // reservation_id|equipment_id|user_id|purpose|start_time|end_time|status
    if (reservation.size() >= 7) {
      if (!response_data.empty())
        response_data += ";";
      response_data += reservation[0] + "|" + reservation[1] + "|" +
                       reservation[2] + "|" + reservation[3] + "|" +
                       reservation[4] + "|" + reservation[5] + "|" +
                       reservation[6];
    }
  }

  std::vector<char> response =
      ProtocolParser::build_reservation_query_response(true, response_data);
  send(fd, response.data(), response.size(), 0);
  std::cout << "返回预约查询结果: " << reservations.size() << " 条记录"
            << std::endl;
}

void EquipmentManagementServer::handle_reservation_approve(
    int fd, const std::string &admin_id, const std::string &payload) {
  std::cout << "处理预约审批: admin=" << admin_id << " payload: " << payload
            << std::endl;

  // 解析payload格式: "reservation_id|action" (action: approve/reject)
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "审批数据格式错误" << std::endl;
    std::vector<char> response =
        ProtocolParser::build_reservation_approve_response(false,
                                                           "数据格式错误");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  std::string reservation_id_str = parts[0];
  std::string action = parts[1];

  // 验证管理员权限
  if (!validate_admin_permission(admin_id)) {
    std::vector<char> response =
        ProtocolParser::build_reservation_approve_response(false, "权限不足");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  int reservation_id;
  try {
    reservation_id = std::stoi(reservation_id_str);
  } catch (const std::exception &e) {
    std::cout << "预约ID格式错误: " << reservation_id_str << std::endl;
    std::vector<char> response =
        ProtocolParser::build_reservation_approve_response(false,
                                                           "预约ID格式错误");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  std::string new_status;

  if (action == "approve") {
    new_status = "approved";
  } else if (action == "reject") {
    new_status = "rejected";
  } else {
    std::vector<char> response =
        ProtocolParser::build_reservation_approve_response(false, "无效操作");
    send(fd, response.data(), response.size(), 0);
    return;
  }

  // 更新数据库
  if (db_manager_->is_connected()) {
    bool success =
        db_manager_->update_reservation_status(reservation_id, new_status);
    if (success) {
      std::vector<char> response =
          ProtocolParser::build_reservation_approve_response(true,
                                                             "审批操作成功");
      send(fd, response.data(), response.size(), 0);
      std::cout << "预约审批成功: reservation " << reservation_id << " -> "
                << new_status << std::endl;
    } else {
      std::vector<char> response =
          ProtocolParser::build_reservation_approve_response(false,
                                                             "数据库错误");
      send(fd, response.data(), response.size(), 0);
    }
  } else {
    std::vector<char> response =
        ProtocolParser::build_reservation_approve_response(false, "系统错误");
    send(fd, response.data(), response.size(), 0);
  }
}

bool EquipmentManagementServer::validate_user_exists(int user_id) {
  // 简化实现，实际应该查询数据库
  // 这里假设用户ID 1-100存在
  return user_id > 0 && user_id <= 100;
}

bool EquipmentManagementServer::validate_admin_permission(
    const std::string &admin_id) {
  // 简化实现，实际应该查询数据库验证管理员权限
  // 这里假设admin_id为"admin"的有权限
  return admin_id == "admin";
}

bool EquipmentManagementServer::check_reservation_conflict(
    const std::string &equipment_id, const std::string &start_time,
    const std::string &end_time) {
  if (db_manager_->is_connected()) {
    return db_manager_->check_reservation_conflict(equipment_id, start_time,
                                                   end_time);
  }
  return false; // 数据库不可用时默认无冲突
}

// 实现公共控制接口
bool EquipmentManagementServer::send_control_command(
    const std::string &equipment_id,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {

  return connections_manager_->send_control_to_simulator(
      equipment_id, command_type, parameters);
}

bool EquipmentManagementServer::send_batch_control_command(
    const std::vector<std::string> &equipment_ids,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {

  return connections_manager_->send_batch_control_to_simulator(
      equipment_ids, command_type, parameters);
}

std::vector<std::string>
EquipmentManagementServer::get_equipment_control_capabilities(
    const std::string &equipment_id) {

  return equipment_manager_->get_equipment_capabilities(equipment_id);
}