#include "simulator_database_reader.h"
#include <iostream>

SimulatorDatabaseReader::SimulatorDatabaseReader()
    : mysql_conn_(nullptr), port_(3306) {
  mysql_conn_ = mysql_init(nullptr);
}

SimulatorDatabaseReader::~SimulatorDatabaseReader() { disconnect(); }

bool SimulatorDatabaseReader::connect(const std::string &host,
                                      const std::string &user,
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
    std::cerr << "模拟器数据库连接失败: " << mysql_error(mysql_conn_)
              << std::endl;
    return false;
  }

  std::cout << "模拟器数据库连接成功" << std::endl;
  return true;
}

void SimulatorDatabaseReader::disconnect() {
  if (mysql_conn_) {
    mysql_close(mysql_conn_);
    mysql_conn_ = nullptr;
  }
}

bool SimulatorDatabaseReader::is_connected() const {
  return mysql_conn_ != nullptr && mysql_ping(mysql_conn_) == 0;
}

std::vector<std::vector<std::string>>
SimulatorDatabaseReader::load_real_equipments() {
  std::string query =
      "SELECT equipment_id, equipment_name, equipment_type, location, "
      "manufacturer, model, register_status "
      "FROM real_equipments "
      "WHERE register_status IN ('registered', 'pending')";

  return execute_query(query);
}

std::vector<std::vector<std::string>>
SimulatorDatabaseReader::execute_query(const std::string &query) {
  std::vector<std::vector<std::string>> results;

  if (mysql_query(mysql_conn_, query.c_str()) != 0) {
    std::cerr << "模拟器查询执行失败: " << mysql_error(mysql_conn_)
              << std::endl;
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

std::string SimulatorDatabaseReader::get_last_error() const {
  return mysql_error(mysql_conn_);
}