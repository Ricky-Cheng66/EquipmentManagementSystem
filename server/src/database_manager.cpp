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

bool DatabaseManager::get_user_info(const std::string &username,
                                    std::string &password_hash,
                                    std::string &role, int &user_id) {
  std::string query =
      "SELECT id, password_hash, role FROM users WHERE username = '" +
      username + "'";
  auto results = execute_query(query);

  if (!results.empty() && results[0].size() >= 3) {
    user_id = std::stoi(results[0][0]);
    password_hash = results[0][1];
    role = results[0][2];
    return true;
  }
  return false;
}

// 获取老师的学生ID列表
std::vector<int> DatabaseManager::get_students_of_teacher(int teacher_id) {
  std::vector<int> student_ids;
  std::string query =
      "SELECT id FROM users WHERE teacher_id = " + std::to_string(teacher_id);
  auto results = execute_query(query);
  for (const auto &row : results) {
    if (!row.empty())
      student_ids.push_back(std::stoi(row[0]));
  }
  return student_ids;
}

// 判断师生关系
bool DatabaseManager::is_teacher_of_student(int teacher_id, int student_id) {
  std::string query =
      "SELECT COUNT(*) FROM users WHERE id = " + std::to_string(student_id) +
      " AND teacher_id = " + std::to_string(teacher_id);
  auto results = execute_query(query);
  return (!results.empty() && std::stoi(results[0][0]) > 0);
}

// 按角色查询预约
std::vector<std::vector<std::string>>
DatabaseManager::get_reservations_for_user(int user_id,
                                           const std::string &role) {
  std::string query;
  if (role == "admin") {
    // 管理员需要看到所有状态为 pending_admin 的预约，包括申请人的角色
    query = "SELECT r.id, r.place_id, r.user_id, r.purpose, r.start_time, "
            "r.end_time, r.status, u.role "
            "FROM reservations r "
            "JOIN users u ON r.user_id = u.id "
            "WHERE r.status = 'pending_admin' ORDER BY r.start_time";
  } else if (role == "teacher") {
    auto student_ids = get_students_of_teacher(user_id);
    if (student_ids.empty()) {
      // 只有老师自己的申请
      query = "SELECT r.id, r.place_id, r.user_id, r.purpose, r.start_time, "
              "r.end_time, r.status, u.role "
              "FROM reservations r "
              "JOIN users u ON r.user_id = u.id "
              "WHERE r.user_id = " +
              std::to_string(user_id) + " ORDER BY r.start_time";
    } else {
      std::string ids_str;
      for (size_t i = 0; i < student_ids.size(); ++i) {
        if (i > 0)
          ids_str += ",";
        ids_str += std::to_string(student_ids[i]);
      }
      query = "SELECT r.id, r.place_id, r.user_id, r.purpose, r.start_time, "
              "r.end_time, r.status, u.role "
              "FROM reservations r "
              "JOIN users u ON r.user_id = u.id "
              "WHERE r.user_id = " +
              std::to_string(user_id) + " OR (r.user_id IN (" + ids_str +
              ") AND r.status = 'pending_teacher') "
              "ORDER BY r.start_time";
    }
  } else { // student
    query = "SELECT r.id, r.place_id, r.user_id, r.purpose, r.start_time, "
            "r.end_time, r.status, u.role "
            "FROM reservations r "
            "JOIN users u ON r.user_id = u.id "
            "WHERE r.user_id = " +
            std::to_string(user_id) + " ORDER BY r.start_time";
  }
  return execute_query(query);
}

std::vector<std::vector<std::string>>
DatabaseManager::get_reservations_by_place_for_user(const std::string &place_id,
                                                    int user_id,
                                                    const std::string &role) {

  // 如果请求的是全部场所，直接调用已有的
  // get_reservations_for_user（该函数也需要修改）
  if (place_id == "all") {
    return get_reservations_for_user(user_id, role);
  }

  std::string query;
  std::string base = "SELECT r.id, r.place_id, r.user_id, r.purpose, "
                     "r.start_time, r.end_time, r.status, u.role "
                     "FROM reservations r "
                     "JOIN users u ON r.user_id = u.id "
                     "WHERE r.place_id = '" +
                     place_id + "'";

  if (role == "admin") {
    query = base + " ORDER BY r.start_time";
  } else if (role == "teacher") {
    auto student_ids = get_students_of_teacher(user_id);
    if (!student_ids.empty()) {
      std::string ids_str;
      for (size_t i = 0; i < student_ids.size(); ++i) {
        if (i > 0)
          ids_str += ",";
        ids_str += std::to_string(student_ids[i]);
      }
      query = base + " AND (r.user_id = " + std::to_string(user_id) +
              " OR (r.user_id IN (" + ids_str +
              ") AND r.status = 'pending_teacher')) "
              "ORDER BY r.start_time";
    } else {
      query = base + " AND r.user_id = " + std::to_string(user_id) +
              " ORDER BY r.start_time";
    }
  } else { // student
    query = base + " AND r.user_id = " + std::to_string(user_id) +
            " ORDER BY r.start_time";
  }

  return execute_query(query);
}

bool DatabaseManager::add_reservation(const std::string &place_id, int user_id,
                                      const std::string &purpose,
                                      const std::string &start_time,
                                      const std::string &end_time,
                                      const std::string &status) {
  std::string query = "INSERT INTO reservations (place_id, user_id, purpose, "
                      "start_time, end_time, status) "
                      "VALUES ('" +
                      place_id + "', " + std::to_string(user_id) + ", '" +
                      purpose + "', '" + start_time + "', '" + end_time +
                      "', '" + status + "')";
  std::cout << "DEBUG: SQL = " << query << std::endl;
  return execute_update(query);
}

bool DatabaseManager::update_reservation_status(int reservation_id,
                                                const std::string &status,
                                                const std::string &place_id) {
  // ✅ 改动：增加place_id条件，确保只能审批指定场所的预约
  std::string query = "UPDATE reservations SET status = '" + status +
                      "' WHERE id = " + std::to_string(reservation_id) +
                      " AND place_id = '" + place_id + "'";

  bool success = execute_update(query);

  if (success && mysql_affected_rows(mysql_conn_) == 0) {
    std::cout << "未找到匹配的预约记录: id=" << reservation_id
              << ", place_id=" << place_id << std::endl;
    return false;
  }

  return success;
}

std::vector<std::vector<std::string>>
DatabaseManager::get_reservations_by_place(const std::string &place_id) {
  std::string query =
      "SELECT id, place_id, user_id, purpose, start_time, end_time, status "
      "FROM reservations WHERE place_id = '" +
      place_id +
      "' "
      "ORDER BY start_time";
  return execute_query(query);
}

std::vector<std::vector<std::string>> DatabaseManager::get_all_reservations() {
  std::string query = "SELECT r.id, r.place_id, r.user_id, r.purpose, "
                      "r.start_time, r.end_time, r.status, u.role "
                      "FROM reservations r "
                      "JOIN users u ON r.user_id = u.id "
                      "ORDER BY r.start_time";
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

std::vector<std::vector<std::string>> DatabaseManager::get_all_places() {
  std::string sql = "SELECT place_id, place_name FROM places ORDER BY place_id";
  return execute_query(sql);
}

std::vector<std::string>
DatabaseManager::get_equipment_ids_by_place(const std::string &place_id) {

  std::string sql =
      "SELECT equipment_ids FROM places WHERE place_id = '" + place_id + "'";
  auto results = execute_query(sql);
  std::vector<std::string> equipment_ids;

  if (!results.empty()) {
    std::string ids_str = results[0][0];
    // 解析逗号分隔的字符串
    size_t pos = 0;
    while ((pos = ids_str.find(',')) != std::string::npos) {
      equipment_ids.push_back(ids_str.substr(0, pos));
      ids_str.erase(0, pos + 1);
    }
    if (!ids_str.empty()) {
      equipment_ids.push_back(ids_str);
    }
  }
  return equipment_ids;
}

bool DatabaseManager::check_place_reservation_conflict(
    const std::string &place_id, const std::string &start_time,
    const std::string &end_time) {
  // 使用开区间判断重叠：start < 请求结束 AND 请求开始 < end
  std::string sql =
      "SELECT COUNT(*) FROM reservations WHERE place_id = '" + place_id +
      "' "
      "AND status IN ('pending_teacher', 'pending_admin', 'approved') "
      "AND start_time < '" +
      end_time + "' AND end_time > '" + start_time + "'";

  // 添加调试输出
  std::cout << "[Conflict Check] SQL: " << sql << std::endl;

  auto results = execute_query(sql);
  int count = results.empty() ? 0 : std::stoi(results[0][0]);
  std::cout << "[Conflict Check] Found " << count << " conflicting records"
            << std::endl;

  // 如果有冲突，可以进一步打印冲突详情（可选）
  if (count > 0) {
    std::string detail_sql =
        "SELECT id, start_time, end_time, status FROM reservations WHERE "
        "place_id = '" +
        place_id +
        "' AND status IN ('pending_teacher','pending_admin','approved') AND "
        "start_time < '" +
        end_time + "' AND end_time > '" + start_time + "'";
    auto details = execute_query(detail_sql);
    for (const auto &row : details) {
      std::cout << "  Conflict: id=" << row[0] << ", start=" << row[1]
                << ", end=" << row[2] << ", status=" << row[3] << std::endl;
    }
  }

  return count > 0;
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

int DatabaseManager::insert_alarm(const std::string &alarm_type,
                                  const std::string &equipment_id,
                                  const std::string &severity,
                                  const std::string &message) {
  if (!is_connected())
    return 0;
  std::string sql =
      "INSERT INTO alarms (alarm_type, equipment_id, severity, message) "
      "VALUES ('" +
      alarm_type + "', '" + equipment_id + "', '" + severity + "', '" +
      message + "')";
  if (mysql_query(mysql_conn_, sql.c_str()) != 0) {
    std::cerr << "插入告警失败: " << mysql_error(mysql_conn_) << std::endl;
    return 0;
  }
  // 返回新插入的自增ID
  return mysql_insert_id(mysql_conn_);
}

bool DatabaseManager::update_alarm_acknowledged(int alarm_id) {
  if (!is_connected())
    return false;
  std::string sql = "UPDATE alarms SET is_acknowledged = TRUE WHERE id = " +
                    std::to_string(alarm_id);
  if (mysql_query(mysql_conn_, sql.c_str()) != 0) {
    std::cerr << "更新告警确认状态失败: " << mysql_error(mysql_conn_)
              << std::endl;
    return false;
  }
  return mysql_affected_rows(mysql_conn_) > 0;
}

std::vector<std::vector<std::string>>
DatabaseManager::get_unacknowledged_alarms() {
  std::string sql =
      "SELECT id, alarm_type, equipment_id, severity, message, created_time "
      "FROM alarms WHERE is_acknowledged = FALSE ORDER BY created_time DESC "
      "LIMIT 50";
  return execute_query(sql);
}