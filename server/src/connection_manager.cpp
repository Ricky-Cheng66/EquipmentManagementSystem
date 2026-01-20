#include "connection_manager.h"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
void ConnectionManager::set_user_info(int fd, const std::string &username,
                                      const std::string &role, int user_id) {
  std::unique_lock lock(connection_rw_lock_);
  UserInfo info;
  info.username = username;
  info.role = role;
  info.user_id = user_id;
  fd_to_user_info_[fd] = info;
}

bool ConnectionManager::get_user_info(int fd, UserInfo &user_info) {
  std::shared_lock lock(connection_rw_lock_);
  auto it = fd_to_user_info_.find(fd);
  if (it != fd_to_user_info_.end()) {
    user_info = it->second;
    return true;
  }
  return false;
}

// 连接管理
void ConnectionManager::add_connection(int fd,
                                       std::shared_ptr<Equipment> equipment,
                                       ProtocolParser::ClientType client_type) {
  std::unique_lock lock(connection_rw_lock_);
  auto it1 = connections_.find(fd);
  if (fd < 0) {
    std::cout << "fd invalid..." << std::endl;
    return;
  } else if (it1 != connections_.end()) {
    std::cout << "fd already in connections_..." << std::endl;
    return;
  } else {
    //添加到映射
    auto [it2, inserted2] = connections_.emplace(fd, equipment);
    if (inserted2) {
      std::cout << "connections_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "connections_ insert failed..." << std::endl;
    }

    auto [it3, inserted3] = heartbeat_times_.emplace(fd, time(nullptr));
    if (inserted3) {
      std::cout << "heartbeat_times_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "heartbeat_times_ insert failed..." << std::endl;
    }

    auto [it4, inserted4] = client_types_.emplace(fd, client_type);
    if (inserted4) {
      std::cout << "client_types_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "equipment_to_fd_ insert failed..." << std::endl;
    }
    auto [it5, inserted5] = connection_healthy_.emplace(fd, true);
    if (inserted5) {
      std::cout << "connection_healthy_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "connection_healthy_insert failed..." << std::endl;
    }
  }
  // 只有设备端连接且设备指针不为空时才加入equipment_to_fd_映射
  if (client_type == ProtocolParser::CLIENT_EQUIPMENT && equipment) {
    auto [it6, inserted6] =
        equipment_to_fd_.emplace(equipment->get_equipment_id(), fd);
    if (inserted6) {
      std::cout << "equipment_to_fd_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "connection_healthy_insert failed..." << std::endl;
    }
    std::cout << "设备连接添加: fd=" << fd << " -> "
              << equipment->get_equipment_id() << std::endl;
  } else {
    std::cout << "Qt客户端连接添加: fd=" << fd << std::endl;
  }
}
void ConnectionManager::remove_connection(int fd) {
  std::unique_lock lock(connection_rw_lock_);
  auto it = connections_.find(fd);
  if (fd < 0) {
    std::cout << "fd invalid..." << std::endl;
  } else if (it != connections_.end()) {
    // 获取连接类型和设备指针
    auto client_type_it = client_types_.find(fd);
    ProtocolParser::ClientType client_type = ProtocolParser::CLIENT_UNKNOWN;
    if (client_type_it != client_types_.end()) {
      client_type = client_type_it->second;
    }

    // 如果是设备连接，从equipment_to_fd_中移除
    if (client_type == ProtocolParser::CLIENT_EQUIPMENT && it->second) {
      // 安全地获取设备ID
      std::string equipment_id = it->second->get_equipment_id();
      equipment_to_fd_.erase(equipment_id);
      std::cout << "移除设备连接映射: " << equipment_id << " -> fd=" << fd
                << std::endl;
    }
    //从所有映射中移除
    connections_.erase(it);
    heartbeat_times_.erase(fd);
    connection_healthy_.erase(fd);
    client_types_.erase(fd);
    close(fd);
    std::cout << "连接完全清理: fd=" << fd << std::endl;
  } else {
    std::cout << "remove_connection failed fd not in connections_..."
              << std::endl;
  }
}

void ConnectionManager::close_all_connections() {
  std::unique_lock lock(connection_rw_lock_);

  // 关闭所有文件描述符
  for (const auto &[fd, equipment] : connections_) {
    close(fd);
  }

  // 清空所有映射
  connections_.clear();
  heartbeat_times_.clear();
  equipment_to_fd_.clear();
  connection_healthy_.clear();
  client_types_.clear();

  std::cout << "所有连接已关闭" << std::endl;
}

// 设备查找
int ConnectionManager::get_fd_by_equipment_id(const std::string &equipment_id) {
  std::shared_lock lock(connection_rw_lock_);
  auto it = equipment_to_fd_.find(equipment_id);
  return (it != equipment_to_fd_.end()) ? it->second : -1;
}
std::shared_ptr<Equipment> ConnectionManager::get_equipment_by_fd(int fd) {
  std::shared_lock lock(connection_rw_lock_);

  // 先检查fd是否有效
  if (fd <= 0) {
    return nullptr;
  }
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    return it->second; // 可能为nullptr（Qt连接）
  }
  return nullptr;
}

std::shared_ptr<Equipment>
ConnectionManager::get_equipment_by_id(const std::string &equipment_id) {
  std::shared_lock lock(connection_rw_lock_);
  for (const auto &[fd, equipment] : connections_) {
    if (equipment->get_equipment_id() == equipment_id) {
      return equipment;
    }
  }
  return nullptr;
}
std::vector<std::shared_ptr<Equipment>>
ConnectionManager::get_all_equipments() {
  std::shared_lock lock(connection_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> all_equipments{};
  for (const auto &[fd, equipment] : connections_) {
    all_equipments.emplace_back(equipment);
  }
  return all_equipments;
}

// 更新心跳时间
void ConnectionManager::update_heartbeat(int fd) {
  std::unique_lock lock(connection_rw_lock_);
  auto it = heartbeat_times_.find(fd);
  if (it != heartbeat_times_.end()) {
    it->second = time(nullptr);
    // 标记连接为健康
    connection_healthy_[fd] = true;
    // 同时更新设备的心跳时间
    auto equip_it = connections_.find(fd);
    if (equip_it != connections_.end()) {
      equip_it->second->update_heartbeat();
    }
  }
}

void ConnectionManager::update_qt_client_heartbeat(int fd) {
  std::unique_lock lock(connection_rw_lock_);

  // 1. 更新心跳时间戳
  auto it = heartbeat_times_.find(fd);
  if (it != heartbeat_times_.end()) {
    it->second = time(nullptr);
  }

  // 2. 标记连接健康
  auto healthy_it = connection_healthy_.find(fd);
  if (healthy_it != connection_healthy_.end()) {
    healthy_it->second = true;
  }

  // 3. 关键：不访问Equipment指针！因为Qt客户端的equipment是nullptr
  // 设备心跳的update_heartbeat()会调用equipment->update_heartbeat()，
  // 但Qt客户端没有对应的Equipment对象

  std::cout << "Qt客户端心跳已更新: fd=" << fd << std::endl;
}

// 检查心跳超时
void ConnectionManager::check_heartbeat_timeout(int timeout_seconds) {
  std::unique_lock lock(connection_rw_lock_);
  time_t current_time = time(nullptr);
  std::vector<int> timeout_fds;

  for (const auto &[fd, last_heartbeat] : heartbeat_times_) {
    if (current_time - last_heartbeat > timeout_seconds) {
      std::cout << "心跳超时: fd=" << fd << ", 最后心跳: " << last_heartbeat
                << std::endl;

      // 标记连接为不健康，但不立即移除
      connection_healthy_[fd] = false;
      timeout_fds.push_back(fd);

      // 注意：这里不自动移除连接，由上层逻辑决定是否关闭
      auto equip_it = connections_.find(fd);
      if (equip_it != connections_.end()) {
        std::cout << "连接不健康: " << equip_it->second->get_equipment_id()
                  << std::endl;
      }
    }
    // 这里只记录超时，不自动关闭连接
    // 连接关闭应该由专门的逻辑处理
  }
}

bool ConnectionManager::is_connection_healthy(int fd) const {
  std::shared_lock lock(connection_rw_lock_);
  auto it = connection_healthy_.find(fd);
  return (it != connection_healthy_.end()) ? it->second : false;
}

bool ConnectionManager::is_equipment_connection_healthy(
    const std::string &equipment_id) const {
  std::shared_lock lock(connection_rw_lock_);
  auto fd_it = equipment_to_fd_.find(equipment_id);
  if (fd_it == equipment_to_fd_.end()) {
    return false;
  }

  auto healthy_it = connection_healthy_.find(fd_it->second);
  return (healthy_it != connection_healthy_.end()) ? healthy_it->second : false;
}

//设备连接状态查询
bool ConnectionManager::is_equipment_connected(
    const std::string &equipment_id) const {
  std::shared_lock lock(connection_rw_lock_);
  auto it = equipment_to_fd_.find(equipment_id);
  if (it == equipment_to_fd_.end()) {
    return false;
  }

  int fd = it->second;
  auto healthy_it = connection_healthy_.find(fd);
  return (healthy_it != connection_healthy_.end()) ? healthy_it->second : false;
}

//连接健康状态查询
bool ConnectionManager::is_connection_alive(int fd) const {
  std::shared_lock lock(connection_rw_lock_);
  auto conn_it = connections_.find(fd);
  if (conn_it == connections_.end()) {
    return false;
  }
  auto it = connection_healthy_.find(fd);
  return (it != connection_healthy_.end()) ? it->second : false;
}

size_t ConnectionManager::get_connection_count() const {
  std::shared_lock lock(connection_rw_lock_);
  return connections_.size();
}

void ConnectionManager::print_connections() const {
  std::shared_lock lock(connection_rw_lock_);
  std::cout << "当前连接数: " << connections_.size() << std::endl;
  for (const auto &[fd, equipment] : connections_) {
    bool healthy = connection_healthy_.at(fd);
    auto client_type_it = client_types_.find(fd);
    ProtocolParser::ClientType client_type =
        client_type_it != client_types_.end() ? client_type_it->second
                                              : ProtocolParser::CLIENT_UNKNOWN;

    std::cout << "fd=" << fd << ", 类型=";
    if (client_type == ProtocolParser::CLIENT_EQUIPMENT) {
      std::cout << "设备端";
      if (equipment) {
        std::cout << ", 设备=" << equipment->get_equipment_id()
                  << ", 状态=" << equipment->get_status();
      } else {
        std::cout << ", 设备指针为空";
      }
    } else if (client_type == ProtocolParser::CLIENT_QT_CLIENT) {
      std::cout << "Qt客户端";
    } else {
      std::cout << "未知";
    }
    std::cout << ", 连接健康=" << (healthy ? "是" : "否") << std::endl;
  }
}

bool ConnectionManager::is_connection_exist(int fd) const {
  std::shared_lock lock(connection_rw_lock_);
  return connections_.find(fd) != connections_.end();
}

std::vector<int> ConnectionManager::get_timeout_fds() const {
  std::shared_lock lock(connection_rw_lock_);
  std::vector<int> timeout_fds;
  time_t current_time = time(nullptr);

  for (const auto &[fd, last_heartbeat] : heartbeat_times_) {
    if (current_time - last_heartbeat > 60) { // 60秒超时
      timeout_fds.push_back(fd);
    }
  }
  return timeout_fds;
}

std::vector<int> ConnectionManager::get_qt_client_connections() const {
  std::shared_lock lock(connection_rw_lock_);
  std::vector<int> qt_fds;

  for (const auto &[fd, client_type] : client_types_) {
    if (client_type == ProtocolParser::CLIENT_QT_CLIENT) {
      qt_fds.push_back(fd);
    }
  }

  return qt_fds;
}

time_t ConnectionManager::get_last_heartbeat(int fd) const {
  std::shared_lock lock(connection_rw_lock_);
  auto it = heartbeat_times_.find(fd);
  return (it != heartbeat_times_.end()) ? it->second : 0;
}

void ConnectionManager::mark_connection_unhealthy(int fd) {
  std::unique_lock lock(connection_rw_lock_);
  auto it = connection_healthy_.find(fd);
  if (it != connection_healthy_.end()) {
    it->second = false;
    std::cout << "连接已标记为不健康: fd=" << fd << std::endl;
  }
}

std::vector<std::pair<int, std::shared_ptr<Equipment>>>
ConnectionManager::get_all_connections() const {
  std::shared_lock lock(connection_rw_lock_);
  std::vector<std::pair<int, std::shared_ptr<Equipment>>> result;
  for (const auto &conn : connections_) {
    result.push_back(conn);
  }
  return result;
}

bool ConnectionManager::send_control_to_simulator(
    ProtocolParser::ClientType client_type, const std::string &equipment_id,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {
  std::shared_lock lock(connection_rw_lock_);

  // 查找设备对应的fd
  auto fd_it = equipment_to_fd_.find(equipment_id);
  if (fd_it == equipment_to_fd_.end()) {
    std::cout << "设备未连接: " << equipment_id << std::endl;
    return false;
  }

  int fd = fd_it->second;

  // 检查连接是否健康
  auto healthy_it = connection_healthy_.find(fd);
  if (healthy_it == connection_healthy_.end() || !healthy_it->second) {
    std::cout << "连接不健康，无法发送控制命令: " << equipment_id << std::endl;
    return false;
  }

  // 检查文件描述符是否仍然有效
  if (fd <= 0) {
    std::cout << "无效的文件描述符: " << fd << " for " << equipment_id
              << std::endl;
    connection_healthy_[fd] = false;
    return false;
  }

  // 尝试发送心跳测试连接是否真的可用
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    std::cout << "设置socket选项失败: " << equipment_id << std::endl;
  }

  // 构建控制命令
  std::vector<char> control_msg = ProtocolParser::build_control_command(
      client_type, equipment_id, command_type, parameters);

  // 发送给设备
  ssize_t bytes_sent =
      send(fd, control_msg.data(), control_msg.size(), MSG_NOSIGNAL);

  if (bytes_sent <= 0) {
    std::cout << "控制命令发送失败: " << equipment_id << std::endl;

    // 标记连接为不健康
    connection_healthy_[fd] = false;

    // 如果连接彻底断开，移除连接
    if (errno == EPIPE || errno == ECONNRESET) {
      std::cout << "连接已断开，移除: " << equipment_id << std::endl;
      // 这里不直接移除，由上层调用remove_connection
      return false;
    }
    return false;
  }
  std::cout << "控制命令已发送: " << equipment_id
            << " 命令: " << static_cast<int>(command_type)
            << " 参数: " << parameters << std::endl;
  return true;
}

// 新增：批量控制命令转发
bool ConnectionManager::send_batch_control_to_simulator(
    ProtocolParser::ClientType client_type,
    const std::vector<std::string> &equipment_ids,
    ProtocolParser::ControlCommandType command_type,
    const std::string &parameters) {

  std::shared_lock lock(connection_rw_lock_);
  bool all_success = true;

  for (const auto &equipment_id : equipment_ids) {
    auto fd_it = equipment_to_fd_.find(equipment_id);
    if (fd_it == equipment_to_fd_.end()) {
      std::cout << "设备未连接: " << equipment_id << std::endl;
      all_success = false;
      continue;
    }

    int fd = fd_it->second;

    if (!connection_healthy_.at(fd)) {
      std::cout << "连接不健康，跳过: " << equipment_id << std::endl;
      all_success = false;
      continue;
    }

    std::vector<char> control_msg = ProtocolParser::build_control_command(
        client_type, equipment_id, command_type, parameters);

    ssize_t bytes_sent = send(fd, control_msg.data(), control_msg.size(), 0);
    if (bytes_sent <= 0) {
      std::cout << "控制命令发送失败: " << equipment_id << std::endl;
      connection_healthy_[fd] = false;
      all_success = false;
    } else {
      std::cout << "控制命令已发送: " << equipment_id << std::endl;
    }
  }

  return all_success;
}

ProtocolParser::ClientType ConnectionManager::get_client_type(int fd) const {
  std::shared_lock lock(connection_rw_lock_);
  auto it = client_types_.find(fd);
  if (it != client_types_.end()) {
    return it->second;
  }
  return ProtocolParser::CLIENT_UNKNOWN;
}

bool ConnectionManager::update_connection_to_equipment(
    int fd, std::shared_ptr<Equipment> equipment) {
  std::unique_lock lock(connection_rw_lock_);

  auto conn_it = connections_.find(fd);
  if (conn_it == connections_.end()) {
    return false;
  }

  // 检查当前是否是Qt客户端连接
  auto client_type_it = client_types_.find(fd);
  if (client_type_it == client_types_.end() ||
      client_type_it->second != ProtocolParser::CLIENT_QT_CLIENT) {
    return false; // 不是Qt客户端连接，不能转换
  }

  if (!equipment) {
    return false; // 设备指针不能为空
  }

  // 更新连接类型和设备指针
  conn_it->second = equipment;
  client_type_it->second = ProtocolParser::CLIENT_EQUIPMENT;

  // 添加到equipment_to_fd_映射
  equipment_to_fd_[equipment->get_equipment_id()] = fd;

  std::cout << "连接类型更新为设备: fd=" << fd << " -> "
            << equipment->get_equipment_id() << std::endl;
  return true;
}