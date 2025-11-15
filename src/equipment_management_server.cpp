#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

#include "../include/connection_manager.h"
#include "../include/epoll.h"
#include "../include/equipment_management_server.h"
#include "../include/socket.h"

bool EquipmentManagementServer::init(int server_port) {
  // socket部分
  server_port_ = server_port;
  // create server_fd
  Socket server_socket{};
  server_fd_ = server_socket.create_server_socket();
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
  // 初始化数据库
  if (!initialize_database()) {
    std::cerr << "数据库初始化失败，服务器启动中止" << std::endl;
    return false;
  }

  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    std::cerr << "Epoll初始化失败" << std::endl;
    return false;
  }

  int max_events = ep.get_epoll_max_events();
  struct epoll_event *evs = new epoll_event[max_events]{};
  std::cout << "设备管理服务器启动成功，开始事件循环..." << std::endl;

  //主服务器循环
  while (true) {
    int nfds = ep.wait_events(evs, -1);
    std::cout << "DEBUG: epoll_wait returned " << nfds << " events"
              << std::endl;
    if (nfds < 0) {
      if (errno == EINTR) {
        //被信号中断
        std::cerr << "epoll_wait 被信号中断" << std::endl;
        continue;
      }
      std::cerr << "epoll_wait other errors" << std::endl;
      continue;
    }
    //处理事件
    if (!process_events(nfds, evs)) {
      std::cerr << "事件处理失败..." << std::endl;
      break;
    }
    // 定期执行维护任务（每10次循环执行一次）
    static int loop_count = 0;
    if (++loop_count >= 10) {
      perform_maintenance_tasks();
      loop_count = 0;
    }
  }

  delete[] evs;
  std::cout << "服务器正常退出" << std::endl;
  return true;
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
      std::cerr << "连接错误或挂起，关闭fd: " << event_fd << std::endl;
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
  case ProtocolParser::EQUIPMENT_REGISTER:
    handle_equipment_register(fd, parse_result.equipment_id,
                              parse_result.payload);
    break;
  case ProtocolParser::STATUS_UPDATE:
    handle_status_update(fd, parse_result.equipment_id, parse_result.payload);
    break;
  case ProtocolParser::CONTROL_CMD:
    handle_control_command(fd, parse_result.equipment_id, parse_result.payload);
    break;
  case ProtocolParser::HEARTBEAT:
    handle_heartbeat(fd, parse_result.equipment_id);
    break;
  default:
    std::cout << "收到数据: " << message << " from fd=" << fd << std::endl;
  }
}

//处理设备注册
void EquipmentManagementServer::handle_equipment_register(
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
  ;

  // 解析payload获取设备信息 (格式: "classroom_101|projector")
  auto parts = ProtocolParser::split_string(payload, '|');
  if (parts.size() < 2) {
    std::cout << "设备注册数据格式错误" << std::endl;
    return;
  }
  std::string location = parts[0];
  std::string equipment_type = parts[1];

  //注册到EquipmentManager
  bool success = equipment_manager_->register_equipment(
      equipment_id, equipment_type, location);

  //保存到数据库
  if (success && db_manager_->is_connected()) {
    // 先查询设备是否已存在
    std::string query =
        "SELECT COUNT(*) FROM equipments WHERE equipment_id = '" +
        equipment_id + "'";
    auto result = db_manager_->execute_query(query);

    if (!result.empty() && std::stoi(result[0][0]) > 0) {
      // 设备已存在，更新状态
      bool update_success =
          db_manager_->update_equipment_status(equipment_id, "online", "on");
      if (update_success) {
        std::cout << "设备状态更新成功: " << equipment_id << std::endl;
      }
    } else {
      // 设备不存在，插入新设备
      bool insert_success = db_manager_->add_equipment(
          equipment_id, equipment_id, equipment_type, location);
      if (!insert_success) {
        std::cerr << "设备注册到数据库失败: " << equipment_id << std::endl;
      } else {
        std::cout << "设备注册到数据库成功: " << equipment_id << std::endl;
      }
    }
  }

  //添加到连接管理
  if (success) {
    auto equip = equipment_manager_->get_equipment(equipment_id);
    if (equip) {
      connections_manager_->add_connection(fd, equip);
    }
  }
  // 发送响应
  std::vector<char> response = ProtocolParser::build_register_response(success);
  send(fd, response.data(), response.size(), 0);

  std::cout << "设备注册" << (success ? "成功" : "失败") << ": " << equipment_id
            << std::endl;
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

  //更新设备状态
  equipment_manager_->update_equipment_status(equipment_id, status);
  equipment_manager_->update_equipment_power_state(equipment_id, power_state);

  //更新数据库
  if (db_manager_->is_connected()) {
    //更新设备表的状态
    bool update_success =
        db_manager_->update_equipment_status(equipment_id, status, power_state);

    //记录状态日志
    bool log_success = db_manager_->log_equipment_status(
        equipment_id, status, power_state, additional_data);

    if (!update_success || !log_success) {
      std::cerr << "状态更新到数据库失败" << equipment_id << std::endl;
    }
  }

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

// 处理心跳
void EquipmentManagementServer::handle_heartbeat(
    int fd, const std::string &equipment_id) {
  //更新心跳时间
  connections_manager_->update_heartbeat(fd);

  //发送心跳响应
  std::string response = ProtocolParser::build_heartbeat_response();
  send(fd, response.c_str(), response.length(), 0);

  std::cout << "心跳处理: " << equipment_id << " fd=" << fd << std::endl;
}

void EquipmentManagementServer::check_heartbeat_timeout() {
  connections_manager_->check_heartbeat_timeout(60); // 60秒超时
}

bool EquipmentManagementServer::accept_new_connection() {
  Socket server_socket{};
  int client_fd = server_socket.accept_socket(server_fd_);

  if (client_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // 没有更多连接可接受，不是错误
      return true;
    }
    std::cerr << "accept失败: " << std::endl;
    return false;
  }

  // 设置非阻塞
  Socket client_socket{};
  if (!client_socket.set_nonblock(client_fd)) {
    std::cerr << "设置非阻塞失败: " << client_fd << std::endl;
    close(client_fd);
    return false;
  }

  // 注册到epoll（ET模式）
  Epoll &ep = Epoll::get_instance();
  if (!ep.add_epoll(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP)) {
    std::cerr << "epoll注册失败: " << client_fd << std::endl;
    close(client_fd);
    return false;
  }

  // 获取客户端信息
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  if (getpeername(client_fd, (struct sockaddr *)&client_addr, &client_len) ==
      0) {
    std::cout << "新客户端连接: fd=" << client_fd
              << ", IP=" << inet_ntoa(client_addr.sin_addr)
              << ", Port=" << ntohs(client_addr.sin_port) << std::endl;
  } else {
    std::cout << "新客户端连接: fd=" << client_fd << " (无法获取地址)"
              << std::endl;
  }

  return true;
}

void EquipmentManagementServer::handle_connection_close(int fd) {
  //清理消息缓冲区
  message_buffers_.erase(fd);

  // 从连接管理中移除
  auto equipment = connections_manager_->get_equipment_by_fd(fd);
  if (equipment) {
    std::cout << "设备离线: " << equipment->get_equipment_id() << std::endl;
    // 更新设备状态为离线
    equipment_manager_->update_equipment_status(equipment->get_equipment_id(),
                                                "offline");
  }

  connections_manager_->remove_connection(fd);
  close(fd);
  std::cout << "fd : " << fd << "已关闭" << std::endl;
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