#include "database_manager.h"
#include <iostream>
#include <sstream>

DatabaseManager::DatabaseManager() : mysql_conn_(nullptr), port_(3306) {
  mysql_conn_ = mysql_init(nullptr);
}

DatabaseManager::~DatabaseManager() { disconnect(); }

bool DatabaseManager::connect(const std::string &host, const std::string &user,
                              const std::string &password,
                              const std::string &database, int port) {
  host_ = host;
  user_ = user;
  password_ = password;
  database_ = database;
  port_ = port;

  if (!mysql_real_connect(mysql_conn_, host.c_str(), user.c_str(),
                          password.c_str(), database.c_str(), port, nullptr,
                          0)) {
    std::cerr << "数据库连接失败: " << mysql_error(mysql_conn_) << std::endl;
    return false;
  }

  std::cout << "数据库连接成功" << std::endl;

  // 初始化表结构
  return initialize_tables();
}

void DatabaseManager::disconnect() {
  if (mysql_conn_) {
    mysql_close(mysql_conn_);
    mysql_conn_ = nullptr;
  }
}

bool DatabaseManager::is_connected() const {
  return mysql_conn_ != nullptr && mysql_ping(mysql_conn_) == 0;
}

bool DatabaseManager::add_equipment(const std::string &equipment_id,
                                    const std::string &equipment_name,
                                    const std::string &equipment_type,
                                    const std::string &location) {
  std::string query = "INSERT INTO equipments (equipment_id, equipment_name, "
                      "equipment_type, location) "
                      "VALUES ('" +
                      equipment_id + "', '" + equipment_name + "', '" +
                      equipment_type + "', '" + location + "')";
  return execute_update(query);
}

bool DatabaseManager::update_equipment_status(const std::string &equipment_id,
                                              const std::string &status,
                                              const std::string &power_state) {
  std::string query = "UPDATE equipments SET status = '" + status +
                      "', power_state = '" + power_state +
                      "', updated_time = NOW() WHERE equipment_id = '" +
                      equipment_id + "'";
  return execute_update(query);
}

bool DatabaseManager::update_equipment_power_state(
    const std::string &equipment_id, const std::string &power_state) {
  std::string query = "UPDATE equipments SET power_state = '" + power_state +
                      "', updated_time = NOW() WHERE equipment_id = '" +
                      equipment_id + "'";
  return execute_update(query);
}

bool DatabaseManager::log_equipment_status(const std::string &equipment_id,
                                           const std::string &status,
                                           const std::string &power_state,
                                           const std::string &additional_data) {
  std::string query = "INSERT INTO equipment_status_logs (equipment_id, "
                      "status, power_state, additional_data) "
                      "VALUES ('" +
                      equipment_id + "', '" + status + "', '" + power_state +
                      "', ";

  if (additional_data.empty()) {
    query += "NULL)";
  } else {
    // 转义单引号，防止SQL注入
    std::string escaped_data = additional_data;
    // 简单的转义：将单引号替换为两个单引号
    size_t pos = 0;
    while ((pos = escaped_data.find("'", pos)) != std::string::npos) {
      escaped_data.replace(pos, 1, "''");
      pos += 2;
    }
    query += "'" + escaped_data + "')";
  }
  std::cout << "DEBUG: 执行日志SQL: " << query << std::endl;
  return execute_update(query);
}

std::vector<std::vector<std::string>>
DatabaseManager::execute_query(const std::string &query) {
  std::vector<std::vector<std::string>> results;

  if (mysql_query(mysql_conn_, query.c_str()) != 0) {
    std::cerr << "查询执行失败: " << mysql_error(mysql_conn_) << std::endl;
    return results;
  }

  MYSQL_RES *result = mysql_store_result(mysql_conn_);
  if (!result) {
    return results;
  }

  int num_fields = mysql_num_fields(result);
  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result))) {
    std::vector<std::string> row_data;
    for (int i = 0; i < num_fields; i++) {
      row_data.push_back(row[i] ? row[i] : "NULL");
    }
    results.push_back(row_data);
  }

  mysql_free_result(result);
  return results;
}

bool DatabaseManager::execute_update(const std::string &query) {
  if (mysql_query(mysql_conn_, query.c_str()) != 0) {
    std::cerr << "更新执行失败: " << mysql_error(mysql_conn_) << std::endl;
    return false;
  }
  return true;
}

bool DatabaseManager::initialize_tables() {
  // 这里可以添加创建表的SQL语句
  // 在实际项目中，建议使用独立的SQL脚本文件
  std::cout << "数据库表结构初始化完成" << std::endl;
  return true;
}

std::string DatabaseManager::get_last_error() const {
  return mysql_error(mysql_conn_);
}

bool DatabaseManager::add_reservation(const std::string &equipment_id,
                                      int user_id, const std::string &purpose,
                                      const std::string &start_time,
                                      const std::string &end_time) {
  std::string query = "INSERT INTO reservations (equipment_id, user_id, "
                      "purpose, start_time, end_time, status) "
                      "VALUES ('" +
                      equipment_id + "', " + std::to_string(user_id) + ", '" +
                      purpose + "', '" + start_time + "', '" + end_time +
                      "', 'pending')";
  return execute_update(query);
}

bool DatabaseManager::update_reservation_status(int reservation_id,
                                                const std::string &status) {
  std::string query = "UPDATE reservations SET status = '" + status +
                      "' WHERE id = " + std::to_string(reservation_id);
  return execute_update(query);
}

std::vector<std::vector<std::string>>
DatabaseManager::get_reservations_by_equipment(
    const std::string &equipment_id) {
  std::string query =
      "SELECT id, equipment_id, user_id, purpose, start_time, end_time, status "
      "FROM reservations WHERE equipment_id = '" +
      equipment_id +
      "' "
      "ORDER BY start_time";
  return execute_query(query);
}

std::vector<std::vector<std::string>> DatabaseManager::get_all_reservations() {
  std::string query =
      "SELECT id, equipment_id, user_id, purpose, start_time, end_time, status "
      "FROM reservations ORDER BY start_time";
  return execute_query(query);
}

bool DatabaseManager::check_reservation_conflict(
    const std::string &equipment_id, const std::string &start_time,
    const std::string &end_time) {
  std::string query =
      "SELECT COUNT(*) FROM reservations WHERE equipment_id = '" +
      equipment_id +
      "' "
      "AND status IN ('pending', 'approved') "
      "AND ((start_time BETWEEN '" +
      start_time + "' AND '" + end_time +
      "') "
      "OR (end_time BETWEEN '" +
      start_time + "' AND '" + end_time +
      "') "
      "OR (start_time <= '" +
      start_time + "' AND end_time >= '" + end_time + "'))";

  auto result = execute_query(query);
  if (!result.empty() && std::stoi(result[0][0]) > 0) {
    return true; // 存在冲突
  }
  return false;
}

bool DatabaseManager::insert_power_log(const std::string &equipment_id,
                                       double power_value,
                                       const std::string &timestamp) {
  // 字段名必须是 power_consumption
  std::string sql =
      "INSERT INTO energy_logs (equipment_id, power_consumption, timestamp) "
      "VALUES ('" +
      equipment_id + "', " + std::to_string(power_value) + ", '" + timestamp +
      "')";
  return execute_update(sql);
}

bool DatabaseManager::update_equipment_energy_total(
    const std::string &equipment_id, double energy_increment) {
  std::string sql = "UPDATE equipments SET energy_total = energy_total + " +
                    std::to_string(energy_increment) +
                    " WHERE equipment_id = '" + equipment_id + "'";
  return execute_update(sql);
}

std::string
DatabaseManager::get_energy_statistics_all(const std::string &timeRange,
                                           const std::string &startDate,
                                           const std::string &endDate) {

  std::string groupBy;
  if (timeRange == "day") {
    groupBy = "DATE(timestamp)";
  } else if (timeRange == "month") {
    groupBy = "DATE_FORMAT(timestamp, '%Y-%m')";
  } else if (timeRange == "week") {
    groupBy = "DATE(DATE_SUB(timestamp, INTERVAL WEEKDAY(timestamp) DAY))";
  } else if (timeRange == "year") {
    groupBy = "DATE_FORMAT(timestamp, '%Y')";
  } else {
    groupBy = "DATE(timestamp)"; // 默认按天
  }

  // 查询能耗数据，格式：equipment_id|period|energy|avg_power|cost
  std::string query = "SELECT equipment_id, " + groupBy +
                      " as period, "
                      "ROUND(SUM(power_consumption * 5 / 3600 / 10), 2) as "
                      "energy, " // 5秒间隔，转换为0.1kWh
                      "ROUND(AVG(power_consumption), 2) as avg_power, "
                      "ROUND(SUM(power_consumption * 5 / 3600 / 10) * 0.6, 2) "
                      "as cost " // 假设电费0.6元/0.1kWh
                      "FROM energy_logs "
                      "WHERE timestamp BETWEEN '" +
                      startDate + " 00:00:00' AND '" + endDate +
                      " 23:59:59' "
                      "GROUP BY equipment_id, period "
                      "ORDER BY equipment_id, period";

  auto results = execute_query(query);

  // 修复：如果没有数据，返回错误信息而非空字符串
  if (results.empty()) {
    return "fail|指定时间范围内暂无能耗数据";
  }

  std::stringstream ss;
  for (size_t i = 0; i < results.size(); ++i) {
    if (i > 0)
      ss << ";";
    // 确保每个字段都有值，避免空指针
    for (int j = 0; j < 5; ++j) {
      if (j > 0)
        ss << "|";
      ss << (results[i][j].empty() ? "0" : results[i][j]);
    }
  }

  return ss.str();
}

std::string DatabaseManager::get_energy_statistics_by_equipment(
    const std::string &equipment_id, const std::string &timeRange,
    const std::string &startDate, const std::string &endDate) {

  std::string groupBy;
  if (timeRange == "day") {
    groupBy = "DATE(timestamp)";
  } else if (timeRange == "month") {
    groupBy = "DATE_FORMAT(timestamp, '%Y-%m')";
  } else if (timeRange == "week") {
    groupBy = "DATE(DATE_SUB(timestamp, INTERVAL WEEKDAY(timestamp) DAY))";
  } else if (timeRange == "year") {
    groupBy = "DATE_FORMAT(timestamp, '%Y')";
  } else {
    groupBy = "DATE(timestamp)";
  }

  std::string query =
      "SELECT equipment_id, " + groupBy +
      " as period, "
      "ROUND(SUM(power_consumption * 5 / 3600 / 10), 2) as energy, "
      "ROUND(AVG(power_consumption), 2) as avg_power, "
      "ROUND(SUM(power_consumption * 5 / 3600 / 10) * 0.6, 2) as cost "
      "FROM energy_logs "
      "WHERE equipment_id = '" +
      equipment_id +
      "' "
      "AND timestamp BETWEEN '" +
      startDate + " 00:00:00' AND '" + endDate +
      " 23:59:59' "
      "GROUP BY " +
      groupBy +
      " "
      "ORDER BY period";

  auto results = execute_query(query);

  // 修复：如果没有数据，返回错误信息而非空字符串
  if (results.empty()) {
    return "fail|指定时间范围内暂无能耗数据";
  }

  std::stringstream ss;
  for (size_t i = 0; i < results.size(); ++i) {
    if (i > 0)
      ss << ";";
    for (int j = 0; j < 5; ++j) {
      if (j > 0)
        ss << "|";
      ss << (results[i][j].empty() ? "0" : results[i][j]);
    }
  }

  return ss.str();
}