#pragma once

#include <mysql/mysql.h>
#include <string>
#include <vector>

class SimulatorDatabaseReader {
public:
  SimulatorDatabaseReader();
  ~SimulatorDatabaseReader();

  // 连接管理
  bool connect(const std::string &host, const std::string &user,
               const std::string &password, const std::string &database,
               int port = 3306);
  void disconnect();
  bool is_connected() const;

  // 只读操作 - 读取真实设备信息
  std::vector<std::vector<std::string>> load_real_equipments();

  // 工具函数
  std::string get_last_error() const;

private:
  std::vector<std::vector<std::string>> execute_query(const std::string &query);

  MYSQL *mysql_conn_;
  std::string host_;
  std::string user_;
  std::string password_;
  std::string database_;
  int port_;
};