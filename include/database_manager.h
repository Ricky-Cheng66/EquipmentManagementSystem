#pragma once

#include <memory>
#include <mysql/mysql.h>
#include <string>
#include <vector>

class DatabaseManager {
public:
  DatabaseManager();
  ~DatabaseManager();

  // 连接管理
  bool connect(const std::string &host, const std::string &user,
               const std::string &password, const std::string &database,
               int port = 3306);
  void disconnect();
  bool is_connected() const;

  // 设备相关操作
  bool add_equipment(const std::string &equipment_id,
                     const std::string &equipment_name,
                     const std::string &equipment_type,
                     const std::string &location);
  bool update_equipment_status(const std::string &equipment_id,
                               const std::string &status,
                               const std::string &power_state);
  bool log_equipment_status(const std::string &equipment_id,
                            const std::string &status,
                            const std::string &power_state,
                            const std::string &additional_data = "");

  // 查询操作
  std::vector<std::vector<std::string>> execute_query(const std::string &query);
  bool execute_update(const std::string &query);

  // 工具函数
  std::string get_last_error() const;

private:
  bool initialize_tables(); // 初始化数据库表

  MYSQL *mysql_conn_;
  std::string host_;
  std::string user_;
  std::string password_;
  std::string database_;
  int port_;
};
