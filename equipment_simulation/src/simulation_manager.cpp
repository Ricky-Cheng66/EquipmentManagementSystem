#include "simulation_manager.h"

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
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
  // 确保所有线程都已停止
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }

  if (event_loop_thread_.joinable()) {
    event_loop_thread_.join();
  }

  // 现在可以安全地析构其他成员
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

  if (threads_started_) {
    std::cout << "模拟器线程已经启动过，请先停止" << std::endl;
    return false;
  }

  // 连接所有设备
  if (!connect_all_equipments()) {
    std::cerr << "设备连接失败" << std::endl;
    return false;
  }

  is_running_ = true;
  threads_started_ = true;
  // 重置心跳运行标志
  {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    heartbeat_running_ = true;
  }
  // 先启动事件循环线程
  try {
    event_loop_thread_ = std::thread(&SimulationManager::event_loop, this);
  } catch (const std::exception &e) {
    std::cerr << "启动事件循环线程失败: " << e.what() << std::endl;
    is_running_ = false;
    threads_started_ = false;
    return false;
  }

  // 然后启动心跳线程
  try {
    heartbeat_thread_ = std::thread(&SimulationManager::heartbeat_loop, this);
  } catch (const std::exception &e) {
    std::cerr << "启动心跳线程失败: " << e.what() << std::endl;
    is_running_ = false;
    threads_started_ = false;

    // 停止已启动的线程
    if (event_loop_thread_.joinable()) {
      event_loop_thread_.join();
    }
    return false;
  }

  std::cout << "模拟器启动成功" << std::endl;
  return true;
}

void SimulationManager::stop() {
  if (!is_running_ && !threads_started_) {
    return;
  }

  std::cout << "正在停止模拟器..." << std::endl;
  is_running_ = false;

  // 停止心跳线程
  {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    heartbeat_running_ = false;
  }
  heartbeat_cv_.notify_all(); // 唤醒可能正在等待的心跳线程

  // 先停止心跳线程
  if (heartbeat_thread_.joinable()) {
    std::cout << "等待心跳线程结束..." << std::endl;
    heartbeat_thread_.join();
    std::cout << "心跳线程已停止" << std::endl;
  }

  heartbeat_cv_.notify_all(); // 唤醒可能正在等待的心跳线程

  // 先停止心跳线程
  if (heartbeat_thread_.joinable()) {
    std::cout << "等待心跳线程结束..." << std::endl;
    heartbeat_thread_.join();
    std::cout << "心跳线程已停止" << std::endl;
  }

  // 然后停止事件循环线程
  if (event_loop_thread_.joinable()) {
    std::cout << "等待事件循环线程结束..." << std::endl;
    event_loop_thread_.join();
    std::cout << "事件循环线程已停止" << std::endl;
  }

  // 断开所有设备连接
  disconnect_all_equipments();

  // 重置标志
  threads_started_ = false;

  std::cout << "模拟器已停止" << std::endl;
}

bool SimulationManager::connect_all_equipments() {
  std::cout << "开始连接所有设备到服务器..." << std::endl;

  auto registered_equipments = connections_->get_registered_equipments();
  int success_count = 0;
  int total_count = registered_equipments.size();

  std::cout << "DEBUG: 找到 " << total_count << " 个设备需要连接" << std::endl;
  std::cout << "DEBUG: 已注册设备: " << registered_equipments.size() << " 个"
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

  // 发送上线消息（原注册消息）
  send_online_message(equipment_id);

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
    // 使用较短的超时时间（10ms），避免长时间阻塞
    int nfds = epoll.wait_events(events, 10);

    if (nfds > 0) {
      if (!process_events(nfds, events)) {
        std::cerr << "事件处理失败" << std::endl;
        break;
      }
    } else if (nfds == 0) {
      // 超时，继续检查运行状态
      // 可以在这里执行一些轻量级的维护任务
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

void SimulationManager::heartbeat_loop() {
  std::cout << "心跳线程启动，间隔: " << HEARTBEAT_INTERVAL_SECONDS << "秒"
            << std::endl;

  while (true) {
    // 首先检查是否应该退出
    {
      std::unique_lock<std::mutex> lock(heartbeat_mutex_);
      if (!heartbeat_running_) {
        break;
      }

      // 等待指定间隔或被唤醒
      heartbeat_cv_.wait_for(lock,
                             std::chrono::seconds(HEARTBEAT_INTERVAL_SECONDS),
                             [this] { return !heartbeat_running_; });
    }

    // 再次检查是否应该退出
    if (!heartbeat_running_) {
      break;
    }

    // 发送心跳
    try {
      send_heartbeats();
    } catch (const std::exception &e) {
      std::cerr << "发送心跳时发生异常: " << e.what() << std::endl;
    }
  }

  std::cout << "心跳线程退出" << std::endl;
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
  case ProtocolParser::ONLINE_RESPONSE: // 修改：注册响应->上线响应
    handle_online_response(fd, equipment_id, parse_result.payload == "success");
    break;
  case ProtocolParser::HEARTBEAT_RESPONSE:
    handle_heartbeat_response(fd, equipment_id);
    break;
  case ProtocolParser::CONTROL_COMMAND:
    handle_control_command(fd, equipment_id, parse_result.payload);
    break;
  case ProtocolParser::STATUS_QUERY:
    handle_status_query(fd, equipment_id);
    break;
  default:
    std::cout << "未知消息类型: " << parse_result.type << " from "
              << equipment_id << std::endl;
    break;
  }
}

// 修改：处理上线响应（原注册响应）
void SimulationManager::handle_online_response(int fd,
                                               const std::string &equipment_id,
                                               bool success) {
  if (success) {
    std::cout << "设备上线成功: " << equipment_id << std::endl;
    // 更新设备状态为在线
    connections_->update_equipment_status(equipment_id, "online");

    // 设置默认电源状态为关闭
    connections_->update_equipment_power_state(equipment_id, "off");
    // 添加：记录连接状态
    std::cout << "设备 " << equipment_id << " 已成功上线并保持连接"
              << std::endl;
    // 立即发送一次状态更新，让服务器知道电源状态
    send_status_update_message(equipment_id);
  } else {
    std::cout << "设备上线失败: " << equipment_id << std::endl;
    connections_->update_equipment_status(equipment_id, "offline");
    // 如果上线失败，应该重试连接
    std::cout << "设备上线失败,1秒后重试连接: " << equipment_id << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connect_equipment(equipment_id);
  }
}

void SimulationManager::handle_heartbeat_response(
    int fd, const std::string &equipment_id) {
  // 心跳响应处理，可以更新最后心跳时间等
  std::cout << "收到心跳响应: " << equipment_id << " (fd=" << fd << ")"
            << std::endl;

  // 更新设备心跳时间
  auto equipment = connections_->get_equipment_by_id(equipment_id);
  if (equipment) {
    equipment->update_heartbeat();
  }
}

// 处理控制命令
void SimulationManager::handle_control_command(int fd,
                                               const std::string &equipment_id,
                                               const std::string &payload) {
  std::cout << "执行控制命令: " << equipment_id << " -> " << payload
            << std::endl;

  // 解析控制命令 (格式: "qt_fd|command_type|parameters")
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "控制命令格式错误: " << payload << std::endl;
    send_control_response(fd, equipment_id, false, "format_error");
    return;
  }
  std::cout << payload << std::endl;

  int command_type;
  try {
    command_type = std::stoi(parts[0]);
  } catch (const std::exception &e) {
    std::cout << "控制命令类型解析错误: " << parts[0] << std::endl;
    send_control_response(fd, equipment_id, false, "invalid_command_type");
    return;
  }

  std::string parameters = parts.size() > 1 ? parts[1] : "";
  std::cout << "parameter: " << parameters << std::endl;
  bool success = false;
  std::string result_message = "";

  // 执行控制命令
  switch (static_cast<ProtocolParser::ControlCommandType>(command_type)) {
  case ProtocolParser::TURN_ON:
    success = simulate_turn_on(equipment_id);
    result_message = success ? "设备已开启" : "设备开启失败";
    break;
  case ProtocolParser::TURN_OFF:
    success = simulate_turn_off(equipment_id);
    result_message = success ? "设备已关闭" : "设备关闭失败";
    break;
  case ProtocolParser::RESTART:
    success = simulate_restart(equipment_id);
    result_message = success ? "设备已重启" : "设备重启失败";
    break;
  case ProtocolParser::ADJUST_SETTINGS:
    success = simulate_adjust_settings(equipment_id, parameters);
    result_message = success ? "设置已调整" : "设置调整失败";
    break;
  default:
    result_message = "未知控制命令";
    break;
  }
  std::string parts_after_qt_fd{};
  if (!success) {
    parts_after_qt_fd = "fail";
  }
  parts_after_qt_fd = "success";
  // 发送控制响应
  send_control_response(
      fd, equipment_id, success,
      parameters + '|' + parts_after_qt_fd + '|' +
          get_command_name(
              static_cast<ProtocolParser::ControlCommandType>(command_type)) +
          '|' + result_message);
}

// 新增：处理状态查询
void SimulationManager::handle_status_query(int fd,
                                            const std::string &equipment_id) {
  std::cout << "处理状态查询: " << equipment_id << std::endl;
  send_status_response(equipment_id);
}

// 发送控制响应
void SimulationManager::send_control_response(int fd,
                                              const std::string &equipment_id,
                                              bool success,
                                              const std::string &parameters) {
  std::vector<char> response = ProtocolParser::build_control_response(
      ProtocolParser::CLIENT_EQUIPMENT, equipment_id, success, parameters);

  if (send_message(fd, response)) {
    std::cout << "控制响应已发送: " << equipment_id
              << " 结果: " << (success ? "成功" : "失败") << " parameters "
              << parameters << std::endl;
  } else {
    std::cout << "控制响应发送失败: " << equipment_id << std::endl;
  }
}

// 模拟设备操作
bool SimulationManager::simulate_turn_on(const std::string &equipment_id) {
  std::cout << "模拟开启设备: " << equipment_id << std::endl;
  // 这里可以添加设备特定的开启逻辑
  connections_->update_equipment_power_state(equipment_id, "on");
  return true;
}

bool SimulationManager::simulate_turn_off(const std::string &equipment_id) {
  std::cout << "模拟关闭设备: " << equipment_id << std::endl;
  connections_->update_equipment_power_state(equipment_id, "off");
  return true;
}

bool SimulationManager::simulate_restart(const std::string &equipment_id) {
  std::cout << "模拟重启设备: " << equipment_id << std::endl;
  // 模拟重启过程：先关后开
  connections_->update_equipment_power_state(equipment_id, "off");
  connections_->update_equipment_status(equipment_id, "restarting");

  // 模拟重启延迟
  std::this_thread::sleep_for(std::chrono::seconds(2));

  connections_->update_equipment_power_state(equipment_id, "on");
  connections_->update_equipment_status(equipment_id, "online");
  return true;
}

bool SimulationManager::simulate_adjust_settings(
    const std::string &equipment_id, const std::string &parameters) {
  std::cout << "模拟调整设备设置: " << equipment_id << " 参数: " << parameters
            << std::endl;
  // 这里可以解析参数并执行具体的设置调整
  return true;
}

// 工具函数：获取命令名称
std::string SimulationManager::get_command_name(
    ProtocolParser::ControlCommandType command_type) {
  switch (command_type) {
  case ProtocolParser::TURN_ON:
    return "turn_on";
  case ProtocolParser::TURN_OFF:
    return "turn_off";
  case ProtocolParser::RESTART:
    return "restart";
  case ProtocolParser::ADJUST_SETTINGS:
    return "adjust_settings";
  default:
    return "unknown";
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

void SimulationManager::perform_maintenance_tasks() {
  // 不再使用loop_count_来触发心跳
  // 只处理状态更新打印等任务

  // 每30次循环打印一次状态
  static int loop_count = 0;
  if (++loop_count >= 30) {
    print_status();
    loop_count = 0;
  }
}

void SimulationManager::send_heartbeats() {
  auto connected_equipments = connections_->get_connected_equipments();

  if (connected_equipments.empty()) {
    std::cout << "[" << get_current_time() << "] 没有已连接设备，跳过心跳发送"
              << std::endl;
    return;
  }

  std::cout << "[" << get_current_time() << "] 开始发送心跳，共 "
            << connected_equipments.size() << " 个设备" << std::endl;

  int success_count = 0;
  int fail_count = 0;

  for (const auto &equipment : connected_equipments) {
    std::string equipment_id = equipment->get_equipment_id();

    // 检查设备连接是否有效
    int fd = connections_->get_fd_by_equipment_id(equipment_id);
    if (fd == -1) {
      std::cout << "[" << get_current_time() << "] 设备 " << equipment_id
                << " 文件描述符无效，跳过心跳" << std::endl;
      fail_count++;
      continue;
    }

    // 发送心跳
    if (send_heartbeat_message(equipment_id)) {
      success_count++;
      std::cout << "[" << get_current_time()
                << "] 心跳发送成功: " << equipment_id << " (fd=" << fd << ")"
                << std::endl;
    } else {
      fail_count++;
      std::cout << "[" << get_current_time()
                << "] 心跳发送失败: " << equipment_id << std::endl;
    }

    // 设备之间添加微小延迟，避免同时发送
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::cout << "[" << get_current_time()
            << "] 心跳发送完成 - 成功: " << success_count
            << ", 失败: " << fail_count << std::endl;
}

void SimulationManager::send_status_updates() {
  auto connected_equipments = connections_->get_connected_equipments();
  for (const auto &equipment : connected_equipments) {
    send_status_update_message(equipment->get_equipment_id());
  }
}

std::string SimulationManager::get_current_time() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
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

// 修改：发送上线消息（原注册消息）
bool SimulationManager::send_online_message(const std::string &equipment_id) {
  auto equipment = connections_->get_equipment_by_id(equipment_id);
  if (!equipment) {
    return false;
  }

  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    return false;
  }

  std::vector<char> message = ProtocolParser::build_online_message(
      ProtocolParser::CLIENT_EQUIPMENT, equipment_id, equipment->get_location(),
      equipment->get_equipment_type());

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送上线消息: " << equipment_id << std::endl;
  } else {
    std::cout << "发送上线消息失败: " << equipment_id << std::endl;
  }

  return success;
}

bool SimulationManager::send_heartbeat_message(
    const std::string &equipment_id) {
  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    std::cout << "无法发送心跳，设备未连接: " << equipment_id << std::endl;
    return false;
  }

  std::vector<char> message = ProtocolParser::build_heartbeat_message(
      ProtocolParser::CLIENT_EQUIPMENT, equipment_id);

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送心跳消息: " << equipment_id << std::endl;
  } else {
    std::cout << "发送心跳消息失败: " << equipment_id << std::endl;
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

  std::string power_state = equipment->get_power_state();
  if (power_state.empty()) {
    power_state = "off"; // 默认值
  }

  std::vector<char> message = ProtocolParser::build_status_update_message(
      ProtocolParser::CLIENT_EQUIPMENT, equipment_id, equipment->get_status(),
      equipment->get_power_state(), "simulated_data");

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "[" << get_current_time() << "] 发送状态更新: " << equipment_id
              << " 状态:" << equipment->get_status() << " 电源:" << power_state
              << std::endl;
  } else {
    std::cout << "[" << get_current_time()
              << "] 状态更新发送失败: " << equipment_id << std::endl;
  }

  return success;
}

// 新增：发送状态响应
bool SimulationManager::send_status_response(const std::string &equipment_id) {
  auto equipment = connections_->get_equipment_by_id(equipment_id);
  if (!equipment) {
    return false;
  }

  int fd = connections_->get_fd_by_equipment_id(equipment_id);
  if (fd == -1) {
    return false;
  }

  std::vector<char> message = ProtocolParser::build_status_response(
      ProtocolParser::CLIENT_EQUIPMENT, equipment_id, equipment->get_status(),
      equipment->get_power_state());

  bool success = send_message(fd, message);
  if (success) {
    std::cout << "发送状态响应: " << equipment_id << std::endl;
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