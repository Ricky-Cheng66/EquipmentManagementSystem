#include "connection_manager.h"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
// 连接管理
void ConnectionManager::add_connection(int fd,
                                       std::shared_ptr<Equipment> equipment) {
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

    auto [it4, inserted4] =
        equipment_to_fd_.emplace(equipment->get_equipment_id(), fd);
    if (inserted4) {
      std::cout << "equipment_to_fd_ insert sucess... quipement_id is" << fd
                << std::endl;
    } else {
      std::cout << "equipment_to_fd_ insert failed..." << std::endl;
    }
  }

  std::cout << "连接添加成功: fd=" << fd << " -> "
            << equipment->get_equipment_id() << std::endl;
}
void ConnectionManager::remove_connection(int fd) {
  std::unique_lock lock(connection_rw_lock_);
  auto it = connections_.find(fd);
  if (fd < 0) {
    std::cout << "fd invalid..." << std::endl;
  } else if (it != connections_.end()) {
    //从所有映射中移除
    connections_.erase(it);
    heartbeat_times_.erase(fd);
    equipment_to_fd_.erase(it->second->get_equipment_id());
    close(fd);
    std::cout << "连接移除: fd=" << fd << " -> "
              << it->second->get_equipment_id() << std::endl;
  } else {
    std::cout << "fd not in connections_..." << std::endl;
  }
}

int ConnectionManager::get_fd_by_equipment_id(const std::string &equipment_id) {
  std::shared_lock lock(connection_rw_lock_);
  auto it = equipment_to_fd_.find(equipment_id);
  if (it != equipment_to_fd_.end()) {
    return it->second;
  }
  return -1;
}
std::shared_ptr<Equipment> ConnectionManager::get_equipment_by_fd(int fd) {
  std::shared_lock lock(connection_rw_lock_);
  auto it = connections_.find(fd);
  if (fd < 0) {
    std::cout << "fd invalid..." << std::endl;
    return nullptr;
  } else if (it != connections_.end()) {
    return it->second;
  } else {
    std::cout << "fd not in connections_..." << std::endl;
    return nullptr;
  }
}

// 设备查找
std::shared_ptr<Equipment>
ConnectionManager::get_equipment_by_id(const std::string &equipment_id) {
  std::shared_lock lock(connection_rw_lock_);
  for (const auto &[k, v] : connections_) {
    if (v->get_equipment_id() == equipment_id) {
      return v;
    }
  }
  return nullptr;
}
std::vector<std::shared_ptr<Equipment>>
ConnectionManager::get_all_equipments() {
  std::shared_lock lock(connection_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> all_equipments{};
  for (const auto &[k, v] : connections_) {
    all_equipments.emplace_back(v);
  }
  return all_equipments;
}

// 更新心跳时间
void ConnectionManager::update_heartbeat(int fd) {
  std::unique_lock lock(connection_rw_lock_);
  auto it = heartbeat_times_.find(fd);
  if (it != heartbeat_times_.end()) {
    it->second = time(nullptr);
    // 同时更新设备的心跳时间
    auto equip_it = connections_.find(fd);
    if (equip_it != connections_.end()) {
      equip_it->second->get_last_heartbeat() = it->second;
    }
  }
}

// 检查心跳超时
void ConnectionManager::check_heartbeat_timeout(int timeout_seconds) {
  std::unique_lock lock(connection_rw_lock_);
  time_t current_time = time(nullptr);
  std::vector<int> timeout_fds;

  for (const auto &[k, v] : heartbeat_times_) {
    int fd = k;
    time_t last_heartbeat = v;

    if (current_time - last_heartbeat > timeout_seconds) {
      std::cout << "心跳超时: fd=" << fd << ", 最后心跳: " << last_heartbeat
                << std::endl;
      timeout_fds.push_back(fd);
    }
  }

  // 移除超时连接
  for (int fd : timeout_fds) {
    auto equip_it = connections_.find(fd);
    if (equip_it != connections_.end()) {
      // 更新设备状态为离线
      equip_it->second->get_status() = "offline";
      std::cout << "设备离线: " << equip_it->second->get_equipment_id()
                << std::endl;
    }
    connections_.erase(fd);
    heartbeat_times_.erase(fd);
    close(fd);
  }
}

size_t ConnectionManager::get_connection_count() const {
  std::shared_lock lock(connection_rw_lock_);
  return connections_.size();
}

void ConnectionManager::print_connections() const {
  std::shared_lock lock(connection_rw_lock_);
  std::cout << "当前连接数: " << connections_.size() << std::endl;
  for (const auto &[k, v] : connections_) {
    auto equipment = v;
    std::cout << "fd=" << k << ", 设备=" << equipment->get_equipment_id()
              << ", 状态=" << equipment->get_status() << std::endl;
  }
}

bool ConnectionManager::send_control_to_simulator(
    const std::string &equipment_id,
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

  // 构建控制命令
  std::vector<char> control_msg = ProtocolParser::build_control_command(
      equipment_id, command_type, parameters);

  // 发送给设备
  ssize_t bytes_sent = send(fd, control_msg.data(), control_msg.size(), 0);
  if (bytes_sent <= 0) {
    std::cout << "控制命令发送失败: " << equipment_id << std::endl;
    return false;
  }

  std::cout << "控制命令已发送: " << equipment_id
            << " 命令: " << static_cast<int>(command_type)
            << " 参数: " << parameters << std::endl;
  return true;
}