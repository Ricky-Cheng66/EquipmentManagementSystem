#include "simulation_manager.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

SimulationManager::SimulationManager()
    : connections_(std::make_unique<SimulatorConnections>()) {
  std::cout << "SimulationManager 初始化完成" << std::endl;
}

SimulationManager::~SimulationManager() {
  stop();
  disconnect_all_equipments();
}

bool SimulationManager::initialize(const std::string &server_ip,
                                   uint16_t server_port,
                                   const std::string &db_host,
                                   const std::string &db_user,
                                   const std::string &db_password,
                                   const std::string &db_database,
                                   int db_port) {
  server_ip_ = server_ip;
  server_port_ = server_port;

  // 初始化Epoll单例
  Epoll &epoll = Epoll::get_instance();
  if (!epoll.initialize()) {
    std::cerr << "Epoll 初始化失败" << std::endl;
    return false;
  }

  // 从数据库加载设备信息
  if (!connections_->initialize_from_database(db_host, db_user, db_password,
                                              db_database, db_port)) {
    std::cerr << "设备信息加载失败" << std::endl;
    return false;
  }

  std::cout << "SimulationManager 初始化成功" << std::endl;
  std::cout << "服务器地址: " << server_ip_ << ":" << server_port_ << std::endl;
  connections_->print_statistics();

  return true;
}

bool SimulationManager::start() {
  if (is_running_) {
    std::cout << "模拟器已经在运行" << std::endl;
    return true;
  }

  // 连接所有设备
  if (!connect_all_equipments()) {
    std::cerr << "设备连接失败" << std::endl;
    return false;
  }

  is_running_ = true;
  event_loop_thread_ = std::thread(&SimulationManager::event_loop, this);

  std::cout << "模拟器启动成功" << std::endl;
  return true;
}

void SimulationManager::stop() {
  if (!is_running_) {
    return;
  }

  std::cout << "正在停止模拟器..." << std::endl;
  is_running_ = false;

  if (event_loop_thread_.joinable()) {
    std::cout << "等待事件循环线程结束..." << std::endl;
    event_loop_thread_.join();
    std::cout << "事件循环线程已结束" << std::endl;
  }

  // 断开所有设备连接
  disconnect_all_equipments();

  std::cout << "模拟器已停止" << std::endl;
}

bool SimulationManager::connect_all_equipments() {
  std::cout << "开始连接所有设备到服务器..." << std::endl;

  auto registered_equipments = connections_->get_registered_equipments();
  auto pending_equipments = connections_->get_pending_equipments();

  int success_count = 0;
  int total_count = registered_equipments.size() + pending_equipments.size();

  std::cout << "DEBUG: 找到 " << total_count << " 个设备需要连接" << std::endl;
  std::cout << "DEBUG: 已注册设备: " << registered_equipments.size() << " 个"
            << std::endl;
  std::cout << "DEBUG: 待注册设备: " << pending_equipments.size() << " 个"
            << std::endl;

  // 连接已注册设备
  for (const auto &equipment : registered_equipments) {
    std::cout << "DEBUG: 尝试连接已注册设备: " << equipment->get_equipment_id()
              << std::endl;
    if (connect_equipment(equipment->get_equipment_id())) {
      success_count++;
      std::cout << "DEBUG: ✅ 设备连接成功: " << equipment->get_equipment_id()
                << std::endl;
    } else {
      std::cout << "DEBUG: ❌ 设备连接失败: " << equipment->get_equipment_id()
                << std::endl;
    }
  }

  // 连接待注册设备
  for (const auto &equipment : pending_equipments) {
    std::cout << "DEBUG: 尝试连接待注册设备: " << equipment->get_equipment_id()
              << std::endl;
    if (connect_equipment(equipment->get_equipment_id())) {
      success_count++;
      std::cout << "DEBUG: ✅ 设备连接成功: " << equipment->get_equipment_id()
                << std::endl;
    } else {
      std::cout << "DEBUG: ❌ 设备连接失败: " << equipment->get_equipment_id()
                << std::endl;
    }
  }

  std::cout << "设备连接完成: " << success_count << "/" << total_count
            << " 成功" << std::endl;
  return success_count > 0;
}

bool SimulationManager::connect_equipment(const std::string &equipment_id) {
  std::cout << "DEBUG connect_equipment: 开始连接设备 " << equipment_id
            << std::endl;

  // 检查设备是否已连接
  if (connections_->is_equipment_connected(equipment_id)) {
    std::cout << "DEBUG: 设备已连接: " << equipment_id << std::endl;
    return true;
  }

  std::cout << "DEBUG: 设备未连接，开始创建连接..." << std::endl;
  bool result = create_equipment_connection(equipment_id);
  std::cout << "DEBUG connect_equipment: 设备 " << equipment_id
            << " 连接结果: " << (result ? "成功" : "失败") << std::endl;
  return result;
}

void SimulationManager::disconnect_equipment(const std::string &equipment_id) {
  connections_->close_connection_by_equipment_id(equipment_id);
}

void SimulationManager::disconnect_all_equipments() {
  connections_->close_all_connections();
}

bool SimulationManager::create_equipment_connection(
    const std::string &equipment_id) {
  std::cout << "DEBUG create_equipment_connection: 开始为设备 " << equipment_id
            << " 创建连接" << std::endl;

  // 创建socket
  int fd = Socket::create_socket();
  if (fd < 0) {
    std::cerr << "DEBUG: ❌ 创建socket失败: " << equipment_id << std::endl;
    return false;
  }
  std::cout << "DEBUG: ✅ 创建socket成功, fd=" << fd << std::endl;

  // 设置非阻塞
  if (!Socket::set_nonblock(fd)) {
    std::cerr << "DEBUG: ❌ 设置非阻塞失败: " << equipment_id << std::endl;
    close(fd);
    return false;
  }
  std::cout << "DEBUG: ✅ 设置非阻塞成功" << std::endl;

  // 连接服务器
  std::cout << "DEBUG: 开始连接到服务器 " << server_ip_ << ":" << server_port_
            << std::endl;
  if (!Socket::connect_to_socket(fd, server_ip_, server_port_)) {
    std::cerr << "DEBUG: ❌ 连接服务器失败: " << equipment_id << std::endl;
    close(fd);
    return false;
  }
  std::cout << "DEBUG: ✅ 连接服务器成功" << std::endl;

  // 添加到Epoll
  if (!add_to_epoll(fd)) {
    std::cerr << "DEBUG: ❌ 添加到Epoll失败: " << equipment_id << std::endl;
    close(fd);
    return false;
  }
  std::cout << "DEBUG: ✅ 添加到Epoll成功" << std::endl;

  // 添加到连接管理
  if (!connections_->add_connection(fd, equipment_id)) {
    std::cerr << "DEBUG: ❌ 添加到连接管理失败: " << equipment_id << std::endl;
    Epoll::get_instance().delete_epoll(fd);
    close(fd);
    return false;
  }
  std::cout << "DEBUG: ✅ 添加到连接管理成功" << std::endl;

  std::cout << "设备连接成功: " << equipment_id << " (fd=" << fd << ")"
            << std::endl;

  // 发送注册消息
  send_register_message(equipment_id);

  return true;
}
bool SimulationManager::add_to_epoll(int fd) {
  std::cout << "DEBUG add_to_epoll: 添加fd=" << fd << "到epoll" << std::endl;
  Epoll &epoll = Epoll::get_instance();
  bool result = epoll.add_epoll(fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
  std::cout << "DEBUG add_to_epoll: 结果=" << (result ? "成功" : "失败")
            << std::endl;
  return result;
}

void SimulationManager::event_loop() {
  std::cout << "进入事件循环..." << std::endl;

  Epoll &epoll = Epoll::get_instance();
  int max_events = epoll.get_epoll_max_events();
  struct epoll_event *events = new epoll_event[max_events];

  while (is_running_) {
    // 使用较短的超时时间（100ms），便于及时检查退出条件
    int nfds = epoll.wait_events(events, 100);

    if (nfds > 0) {
      if (!process_events(nfds, events)) {
        std::cerr << "事件处理失败" << std::endl;
        break;
      }
    } else if (nfds == 0) {
      // 超时，继续检查运行状态
      continue;
    } else {
      // 错误处理
      if (errno == EINTR) {
        // 被信号中断，检查是否需要退出
        if (!is_running_)
          break;
        continue;
      } else {
        std::cerr << "epoll_wait 错误: " << strerror(errno) << std::endl;
        break;
      }
    }

    // 定期执行维护任务
    perform_maintenance_tasks();
  }

  delete[] events;
  std::cout << "退出事件循环" << std::endl;
}

bool SimulationManager::process_events(int nfds, struct epoll_event *events) {
  for (int i = 0; i < nfds; i++) {
    int fd = events[i].data.fd;
    uint32_t event_mask = events[i].events;

    // 检查错误事件
    if (event_mask & (EPOLLERR | EPOLLHUP)) {
      std::cerr << "连接错误或挂起,关闭fd: " << fd << std::endl;
      handle_connection_close(fd);
      continue;
    }

    // 处理可读事件
    if (event_mask & EPOLLIN) {
      handle_server_data(fd);
    }
  }
  return true;
}

void SimulationManager::handle_server_data(int fd) {
  char buffer[2048];

  while (true) {
    int bytes_received = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_received > 0) {
      // 追加到应用层缓冲区
      MessageBuffer *msg_buffer = get_message_buffer(fd);
      msg_buffer->append_data(buffer, bytes_received);

      // 解析完整消息
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
      std::cout << "服务器关闭连接: fd=" << fd << std::endl;
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

void SimulationManager::process_single_message(int fd,
                                               const std::string &message) {
  auto equipment = connections_->get_equipment_by_fd(fd);
  if (!equipment) {
    std::cerr << "找不到fd对应的设备: " << fd << std::endl;
    return;
  }

  std::string equipment_id = equipment->get_equipment_id();
  auto parse_result = ProtocolParser::parse_message(message);

  if (!parse_result.success) {
    std::cout << "协议解析失败: " << message << " from " << equipment_id
              << std::endl;
    return;
  }

  std::cout << "收到服务器消息: " << equipment_id
            << " -> 类型: " << parse_result.type
            << " payload: " << parse_result.payload << std::endl;

  switch (parse_result.type) {
  case ProtocolParser::EQUIPMENT_REGISTER:
    // 注册响应
    handle_register_response(fd, equipment_id,
                             parse_result.payload == "success");
    break;

  case ProtocolParser::HEARTBEAT:
    // 心跳响应
    handle_heartbeat_response(fd, equipment_id);
    break;

  case ProtocolParser::CONTROL_CMD:
    // 控制命令
    handle_control_command(fd, equipment_id, parse_result.payload);
    break;

  default:
    std::cout << "未知消息类型: " << parse_result.type << " from "
              << equipment_id << std::endl;
    break;
  }
}

void SimulationManager::handle_connection_close(int fd) {
  auto equipment = connections_->get_equipment_by_fd(fd);
  if (equipment) {
    std::cout << "设备断开连接: " << equipment->get_equipment_id()
              << " (fd=" << fd << ")" << std::endl;
  }

  // 从Epoll中移除
  Epoll::get_instance().delete_epoll(fd);

  // 清理消息缓冲区
  {
    std::unique_lock lock(buffers_rw_lock_);
    message_buffers_.erase(fd);
  }

  // 通过connections类关闭连接
  connections_->close_connection(fd);
}

void SimulationManager::handle_register_response(
    int fd, const std::string &equipment_id, bool success) {
  if (success) {
    std::cout << "设备注册成功: " << equipment_id << std::endl;
    // 更新设备状态为在线
    connections_->update_equipment_status(equipment_id, "online");
    // connections_->update_equipment_power_state(equipment_id, "on");
  } else {
    std::cout << "设备注册失败: " << equipment_id << std::endl;
    connections_->update_equipment_status(equipment_id, "offline");
  }
}

void SimulationManager::handle_heartbeat_response(
    int fd, const std::string &equipment_id) {
  // 心跳响应处理，可以更新最后心跳时间等
  std::cout << "收到心跳响应: " << equipment_id << std::endl;
}

void SimulationManager::handle_control_command(int fd,
                                               const std::string &equipment_id,
                                               const std::string &command) {
  std::cout << "执行控制命令: " << equipment_id << " -> " << command
            << std::endl;

  if (command == "turn_on") {
    connections_->update_equipment_power_state(equipment_id, "on");
    std::cout << "设备已开启: " << equipment_id << std::endl;
  } else if (command == "turn_off") {
    connections_->update_equipment_power_state(equipment_id, "off");
    std::cout << "设备已关闭: " << equipment_id << std::endl;
  } else if (command == "restart") {
    connections_->update_equipment_power_state(equipment_id, "off");
    // 模拟重启延迟
    std::this_thread::sleep_for(std::chrono::seconds(2));
    connections_->update_equipment_power_state(equipment_id, "on");
    std::cout << "设备已重启: " << equipment_id << std::endl;
  }

  // 发送状态更新确认
  send_status_update_message(equipment_id);
}

void SimulationManager::perform_maintenance_tasks() {
  loop_count_++;

  // 发送心跳
  if (loop_count_ % HEARTBEAT_INTERVAL == 0) {
    send_heartbeats();
  }

  // 发送状态更新
  if (loop_count_ % STATUS_UPDATE_INTERVAL == 0) {
    send_status_updates();
  }

  // 发送待注册设备的注册消息
  send_pending_registrations();

  // 每30次循环打印一次状态
  if (loop_count_ % 30 == 0) {
    print_status();
    loop_count_ = 0; // 防止溢出
  }
}

void SimulationManager::send_heartbeats() {
  auto connected_equipments = connections_->get_connected_equipments();
  for (const auto &equipment : connected_equipments) {
    send_heartbeat_message(equipment->get_equipment_id());
  }
}

void SimulationManager::send_status_updates() {
  auto connected_equipments = connections_->get_connected_equipments();
  for (const auto &equipment : connected_equipments) {
    send_status_update_message(equipment->get_equipment_id());
  }
}

void SimulationManager::
    send_pending_registrations() { // 如果已经请求停止，不进行重连
  if (!is_running_) {
    return;
  }
  auto pending_equipments = connections_->get_pending_equipments();
  for (const auto &equipment : pending_equipments) {
    std::string equipment_id = equipment->get_equipment_id();
    // 如果设备未连接，尝试连接
    if (!connections_->is_equipment_connected(equipment_id)) {
      connect_equipment(equipment_id);
    } else {
      // 如果已连接但未注册，发送注册消息
      send_register_message(equipment_id);
    }
    // 短暂延迟，避免过于频繁的重连
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

bool SimulationManager::send_message(int fd, const std::vector<char> &message) {
  ssize_t send_bytes = 0;
  ssize_t message_len = static_cast<ssize_t>(message.size());

  while (send_bytes < message_len) {
    ssize_t n_bytes =
        send(fd, message.data() + send_bytes, message_len - send_bytes, 0);
    if (n_bytes < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        usleep(1000);
        continue;
      }
      std::perror("发送消息失败");
      return false;
    }
    send_bytes += n_bytes;
  }
  return true;
}

bool SimulationManager::send_register_message(const std::string &equipment_id) {
  auto equipment = connections_->get_equipment_by_id(equipment_id);
  if (!equipment) {
    return false;
  }

  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    return false;
  }

  std::vector<char> message = ProtocolParser::build_register_message(
      equipment_id, equipment->get_location(), equipment->get_equipment_type());

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送注册消息: " << equipment_id << std::endl;
  } else {
    std::cout << "发送注册消息失败: " << equipment_id << std::endl;
  }

  return success;
}

bool SimulationManager::send_heartbeat_message(
    const std::string &equipment_id) {
  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    return false;
  }

  std::vector<char> message =
      ProtocolParser::build_heartbeat_message(equipment_id);

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送心跳消息: " << equipment_id << std::endl;
  }

  return success;
}

bool SimulationManager::send_status_update_message(
    const std::string &equipment_id) {
  auto equipment = connections_->get_equipment_by_id(equipment_id);
  if (!equipment) {
    return false;
  }

  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    return false;
  }

  std::vector<char> message = ProtocolParser::build_status_update_message(
      equipment_id, equipment->get_status(), equipment->get_power_state(),
      "simulated_data");

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送状态更新: " << equipment_id << std::endl;
  }

  return success;
}

MessageBuffer *SimulationManager::get_message_buffer(int fd) {
  std::unique_lock lock(buffers_rw_lock_);

  auto it = message_buffers_.find(fd);
  if (it == message_buffers_.end()) {
    auto buffer = std::make_unique<MessageBuffer>();
    auto result = message_buffers_.emplace(fd, std::move(buffer));
    return result.first->second.get();
  }
  return it->second.get();
}

void SimulationManager::print_status() const {
  std::cout << "\n=== 模拟器状态 ===" << std::endl;
  std::cout << "运行状态: " << (is_running_ ? "运行中" : "已停止") << std::endl;
  std::cout << "服务器: " << server_ip_ << ":" << server_port_ << std::endl;
  connections_->print_statistics();
  connections_->print_connections();
  std::cout << "==================" << std::endl;
}