#include "simulator_connections.h"
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

SimulatorConnections::SimulatorConnections()
    : db_reader_(std::make_unique<SimulatorDatabaseReader>()) {
  std::cout << "SimulatorConnections 初始化完成" << std::endl;
}

bool SimulatorConnections::initialize_from_database(const std::string &host,
                                                    const std::string &user,
                                                    const std::string &password,
                                                    const std::string &database,
                                                    int port) {
  // 连接数据库
  if (!db_reader_->connect(host, user, password, database, port)) {
    std::cerr << "数据库连接失败，无法初始化设备信息" << std::endl;
    return false;
  }

  // 读取设备信息
  auto results = db_reader_->load_real_equipments();
  if (results.empty()) {
    std::cout << "未找到需要管理的设备" << std::endl;
    return true; // 没有设备不是错误
  }

  std::unique_lock lock(equipments_rw_lock_);
  int loaded_count = 0;
  int registered_count = 0;

  for (const auto &row : results) {
    if (row.size() < 7)
      continue;

    std::string equipment_id = row[0];
    std::string equipment_name = row[1];
    std::string equipment_type = row[2];
    std::string location = row[3];
    std::string manufacturer = row[4];
    std::string model = row[5];
    std::string register_status = row[6];

    // 只处理已注册的设备
    if (register_status != "registered") {
      continue;
    }
    std::cout << "DEBUG: 开始添加设备 " << equipment_id
              << " 类型: " << equipment_type << " 位置: " << location
              << std::endl;

    // 添加到设备映射
    bool success =
        add_real_equipment(equipment_id, equipment_name, equipment_type,
                           location, register_status, manufacturer, model);
    if (success) {
      loaded_count++;
      registered_count++;

      // 验证设备是否添加成功
      auto it = real_equipments_.find(equipment_id);
      if (it != real_equipments_.end() && it->second.equipment) {
        // 为新添加的设备设置默认电源状态
        it->second.equipment->update_equipment_power_state("off");
        std::cout << "DEBUG: 设备 " << equipment_id
                  << " 添加成功，电源状态已设为off" << std::endl;
      } else {
        std::cerr << "ERROR: 设备 " << equipment_id
                  << " 添加后未找到或equipment指针为空" << std::endl;
      }

      std::cout << "加载设备: " << equipment_id << " [" << equipment_type
                << "] 位置: " << location << " 注册状态: " << register_status
                << std::endl;
    } else {
      std::cerr << "ERROR: 添加设备失败: " << equipment_id << std::endl;
    }
  }

  // 读取完成后可以断开数据库连接，因为模拟器不需要持续访问数据库
  db_reader_->disconnect();

  db_reader_->disconnect();
  std::cout << "成功加载 " << loaded_count << " 个已注册设备" << std::endl;
  return true;
}

bool SimulatorConnections::add_connection(int fd,
                                          const std::string &equipment_id) {
  if (fd < 0) {
    std::cerr << "无效的文件描述符: " << fd << std::endl;
    return false;
  }

  std::shared_lock equip_lock(equipments_rw_lock_);
  std::unique_lock conn_lock(connections_rw_lock_);

  // 检查设备是否存在
  auto equip_it = real_equipments_.find(equipment_id);
  if (equip_it == real_equipments_.end()) {
    std::cerr << "设备不存在: " << equipment_id << std::endl;
    return false;
  }

  // 检查是否已连接
  auto fd_it = fd_to_equipment_.find(fd);
  if (fd_it != fd_to_equipment_.end()) {
    std::cout << "文件描述符 " << fd
              << " 已连接到设备: " << fd_it->second->get_equipment_id()
              << std::endl;
    return false;
  }

  auto equip_fd_it = equipment_to_fd_.find(equipment_id);
  if (equip_fd_it != equipment_to_fd_.end()) {
    std::cout << "设备 " << equipment_id
              << " 已连接到文件描述符: " << equip_fd_it->second << std::endl;
    return false;
  }

  // 添加连接
  auto equipment = equip_it->second.equipment;
  fd_to_equipment_[fd] = equipment;
  equipment_to_fd_[equipment_id] = fd;

  std::cout << "添加连接: fd=" << fd << " -> " << equipment_id << std::endl;
  return true;
}

void SimulatorConnections::remove_connection(int fd) {
  std::unique_lock lock(connections_rw_lock_);

  auto it = fd_to_equipment_.find(fd);
  if (it != fd_to_equipment_.end()) {
    std::string equipment_id = it->second->get_equipment_id();
    fd_to_equipment_.erase(it);
    equipment_to_fd_.erase(equipment_id);
    std::cout << "移除连接: fd=" << fd << " (" << equipment_id << ")"
              << std::endl;
  } else {
    std::cout << "连接不存在: fd=" << fd << std::endl;
  }
}

void SimulatorConnections::remove_connection_by_equipment_id(
    const std::string &equipment_id) {
  std::unique_lock lock(connections_rw_lock_);

  auto it = equipment_to_fd_.find(equipment_id);
  if (it != equipment_to_fd_.end()) {
    int fd = it->second;
    equipment_to_fd_.erase(it);
    fd_to_equipment_.erase(fd);
    std::cout << "移除连接: " << equipment_id << " (fd=" << fd << ")"
              << std::endl;
  } else {
    std::cout << "设备未连接: " << equipment_id << std::endl;
  }
}

// 在原有实现中添加以下方法：

void SimulatorConnections::close_connection(int fd) {
  std::unique_lock lock(connections_rw_lock_);

  auto it = fd_to_equipment_.find(fd);
  if (it != fd_to_equipment_.end()) {
    std::string equipment_id = it->second->get_equipment_id();

    // 从映射中移除
    fd_to_equipment_.erase(it);
    equipment_to_fd_.erase(equipment_id);

    // 关闭文件描述符
    if (fd > 0) {
      close(fd);
    }

    std::cout << "关闭连接: fd=" << fd << " (" << equipment_id << ")"
              << std::endl;
  } else {
    std::cout << "连接不存在，无法关闭: fd=" << fd << std::endl;
  }
}

void SimulatorConnections::close_connection_by_equipment_id(
    const std::string &equipment_id) {
  std::unique_lock lock(connections_rw_lock_);

  auto it = equipment_to_fd_.find(equipment_id);
  if (it != equipment_to_fd_.end()) {
    int fd = it->second;

    // 从映射中移除
    equipment_to_fd_.erase(it);
    fd_to_equipment_.erase(fd);

    // 关闭文件描述符
    if (fd > 0) {
      close(fd);
    }

    std::cout << "关闭连接: " << equipment_id << " (fd=" << fd << ")"
              << std::endl;
  } else {
    std::cout << "设备未连接，无法关闭: " << equipment_id << std::endl;
  }
}

void SimulatorConnections::close_all_connections() {
  std::unique_lock lock(connections_rw_lock_);

  std::cout << "关闭所有连接，共 " << fd_to_equipment_.size() << " 个连接"
            << std::endl;

  // 关闭所有文件描述符
  for (const auto &[fd, equipment] : fd_to_equipment_) {
    if (fd > 0) {
      close(fd);
    }
  }

  // 清空映射
  fd_to_equipment_.clear();
  equipment_to_fd_.clear();
}

bool SimulatorConnections::is_connection_valid(int fd) const {
  std::shared_lock lock(connections_rw_lock_);

  if (fd_to_equipment_.find(fd) == fd_to_equipment_.end()) {
    return false;
  }

  // 检查文件描述符是否仍然有效
  return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

bool SimulatorConnections::has_connection(int fd) const {
  std::shared_lock lock(connections_rw_lock_);
  return fd_to_equipment_.find(fd) != fd_to_equipment_.end();
}

bool SimulatorConnections::is_equipment_connected(
    const std::string &equipment_id) const {
  std::shared_lock lock(connections_rw_lock_);
  return equipment_to_fd_.find(equipment_id) != equipment_to_fd_.end();
}

std::shared_ptr<Equipment> SimulatorConnections::get_equipment_by_fd(int fd) {
  std::shared_lock lock(connections_rw_lock_);

  auto it = fd_to_equipment_.find(fd);
  if (it != fd_to_equipment_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Equipment>
SimulatorConnections::get_equipment_by_id(const std::string &equipment_id) {
  // 使用try_shared_lock避免死锁
  std::unique_lock<std::shared_mutex> lock(equipments_rw_lock_,
                                           std::try_to_lock);

  if (!lock.owns_lock()) {
    // 获取锁失败，返回空指针
    std::cerr << "警告: 无法获取equipments_rw_lock_读取锁" << std::endl;
    return nullptr;
  }

  auto it = real_equipments_.find(equipment_id);
  if (it != real_equipments_.end()) {
    // 检查shared_ptr是否有效
    auto equipment = it->second.equipment;
    if (!equipment) {
      std::cerr << "警告: 设备 " << equipment_id << " 的shared_ptr为空"
                << std::endl;
    }
    return equipment;
  }

  return nullptr;
}

int SimulatorConnections::get_fd_by_equipment_id(
    const std::string &equipment_id) {
  std::shared_lock lock(connections_rw_lock_);

  auto it = equipment_to_fd_.find(equipment_id);
  if (it != equipment_to_fd_.end()) {
    return it->second;
  }
  return -1;
}

std::vector<std::shared_ptr<Equipment>>
SimulatorConnections::get_all_equipments() {
  std::shared_lock lock(equipments_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> equipments;

  for (const auto &[id, info] : real_equipments_) {
    equipments.push_back(info.equipment);
  }
  return equipments;
}

std::vector<std::shared_ptr<Equipment>>
SimulatorConnections::get_registered_equipments() {
  std::shared_lock lock(equipments_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> equipments;

  for (const auto &[id, info] : real_equipments_) {
    if (info.register_status == "registered") {
      equipments.push_back(info.equipment);
    }
  }
  return equipments;
}

std::vector<std::shared_ptr<Equipment>>
SimulatorConnections::get_pending_equipments() {
  std::shared_lock lock(equipments_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> equipments;

  for (const auto &[id, info] : real_equipments_) {
    if (info.register_status == "pending") {
      equipments.push_back(info.equipment);
    }
  }
  return equipments;
}

std::vector<std::shared_ptr<Equipment>>
SimulatorConnections::get_connected_equipments() {
  std::shared_lock lock(connections_rw_lock_);
  std::vector<std::shared_ptr<Equipment>> equipments;

  for (const auto &[fd, equipment] : fd_to_equipment_) {
    equipments.push_back(equipment);
  }
  return equipments;
}

size_t SimulatorConnections::get_connection_count() const {
  std::shared_lock lock(connections_rw_lock_);
  return fd_to_equipment_.size();
}

size_t SimulatorConnections::get_equipment_count() const {
  std::shared_lock lock(equipments_rw_lock_);
  return real_equipments_.size();
}

size_t SimulatorConnections::get_registered_count() const {
  std::shared_lock lock(equipments_rw_lock_);
  size_t count = 0;
  for (const auto &[id, info] : real_equipments_) {
    if (info.register_status == "registered")
      count++;
  }
  return count;
}

size_t SimulatorConnections::get_pending_count() const {
  std::shared_lock lock(equipments_rw_lock_);
  size_t count = 0;
  for (const auto &[id, info] : real_equipments_) {
    if (info.register_status == "pending")
      count++;
  }
  return count;
}

bool SimulatorConnections::update_equipment_status(
    const std::string &equipment_id, const std::string &status) {
  auto equipment = get_equipment_by_id(equipment_id);
  if (equipment) {
    equipment->update_status(status);
    std::cout << "更新设备状态: " << equipment_id << " -> " << status
              << std::endl;
    return true;
  }
  return false;
}

bool SimulatorConnections::update_equipment_power_state(
    const std::string &equipment_id, const std::string &power_state) {
  auto equipment = get_equipment_by_id(equipment_id);
  if (equipment) {
    equipment->update_equipment_power_state(power_state);
    std::cout << "更新设备电源状态: " << equipment_id << " -> " << power_state
              << std::endl;
    return true;
  }
  return false;
}

void SimulatorConnections::batch_update_status(const std::string &status) {
  std::shared_lock lock(equipments_rw_lock_);
  for (auto &[id, info] : real_equipments_) {
    info.equipment->update_status(status);
  }
  std::cout << "批量更新所有设备状态: " << status << std::endl;
}

void SimulatorConnections::batch_update_power_state(
    const std::string &power_state) {
  std::shared_lock lock(equipments_rw_lock_);
  for (auto &[id, info] : real_equipments_) {
    info.equipment->update_equipment_power_state(power_state);
  }
  std::cout << "批量更新所有设备电源状态: " << power_state << std::endl;
}

void SimulatorConnections::print_connections() const {
  std::shared_lock lock(connections_rw_lock_);

  std::cout << "=== 当前连接状态 ===" << std::endl;
  std::cout << "活跃连接数: " << fd_to_equipment_.size() << std::endl;

  for (const auto &[fd, equipment] : fd_to_equipment_) {
    std::string power_state = equipment->get_power_state();
    if (power_state.empty()) {
      power_state = "off"; // 默认值
    }

    std::cout << "fd=" << fd << " -> " << equipment->get_equipment_id() << " ["
              << equipment->get_equipment_type() << "]"
              << " 状态: " << equipment->get_status()
              << " 电源: " << power_state << std::endl;
  }
  std::cout << "===================" << std::endl;
}

void SimulatorConnections::print_all_equipments() const {
  std::shared_lock lock(equipments_rw_lock_);

  std::cout << "=== 所有设备信息 ===" << std::endl;
  std::cout << "设备总数: " << real_equipments_.size() << std::endl;

  for (const auto &[id, info] : real_equipments_) {
    auto equipment = info.equipment;
    std::cout << "设备ID: " << id
              << " 类型: " << equipment->get_equipment_type()
              << " 位置: " << equipment->get_location()
              << " 注册状态: " << info.register_status << " 连接状态: "
              << (is_equipment_connected(id) ? "已连接" : "未连接")
              << " 运行状态: " << equipment->get_status()
              << " 电源: " << equipment->get_power_state() << std::endl;
  }
  std::cout << "===================" << std::endl;
}

void SimulatorConnections::print_statistics() const {
  std::shared_lock equip_lock(equipments_rw_lock_);
  std::shared_lock conn_lock(connections_rw_lock_);

  std::cout << "=== 系统统计 ===" << std::endl;
  std::cout << "总设备数: " << real_equipments_.size() << std::endl;
  std::cout << "已注册设备: " << get_registered_count() << std::endl;
  std::cout << "活跃连接: " << fd_to_equipment_.size() << std::endl;
  std::cout << "=================" << std::endl;
}

bool SimulatorConnections::add_real_equipment(
    const std::string &equipment_id, const std::string &equipment_name,
    const std::string &equipment_type, const std::string &location,
    const std::string &register_status, const std::string &manufacturer,
    const std::string &model) {
  // 检查设备是否已存在
  auto it = real_equipments_.find(equipment_id);
  if (it != real_equipments_.end()) {
    std::cout << "设备已存在: " << equipment_id << std::endl;
    return false;
  }

  try {
    // 创建设备对象，初始状态为离线，电源关闭
    auto equipment =
        std::make_shared<Equipment>(equipment_id, equipment_type, location,
                                    "offline", // 初始状态
                                    "off"      // 初始电源状态
        );

    // 设置设备信息
    EquipmentInfo info;
    info.equipment = equipment;
    info.register_status = register_status;
    info.manufacturer = manufacturer;
    info.model = model;

    // 添加到映射
    real_equipments_[equipment_id] = info;

    std::cout << "成功添加设备: " << equipment_id << " 类型: " << equipment_type
              << " 位置: " << location << std::endl;

    return true;

  } catch (const std::exception &e) {
    std::cerr << "创建设备对象失败 (" << equipment_id << "): " << e.what()
              << std::endl;
    return false;
  }
}