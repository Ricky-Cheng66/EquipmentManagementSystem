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

  bool update_equipment_power_state(const std::string &equipment_id,
                                    const std::string &power_state);

  bool log_equipment_status(const std::string &equipment_id,
                            const std::string &status,
                            const std::string &power_state,
                            const std::string &additional_data = "");

  // 新增：记录功耗日志
  bool insert_power_log(const std::string &equipment_id, double power_value,
                        const std::string &timestamp);

  // 新增：累加设备总能耗
  bool update_equipment_energy_total(const std::string &equipment_id,
                                     double energy_increment);

  // 能耗统计查询（所有设备）
  std::string get_energy_statistics_all(const std::string &timeRange,
                                        const std::string &startDate,
                                        const std::string &endDate);

  // 能耗统计查询（单个设备）
  std::string get_energy_statistics_by_equipment(
      const std::string &equipment_id, const std::string &timeRange,
      const std::string &startDate, const std::string &endDate);

  // 告警相关操作
  int insert_alarm(const std::string &alarm_type,
                   const std::string &equipment_id, const std::string &severity,
                   const std::string &message);

  bool update_alarm_acknowledged(int alarm_id);

  std::vector<std::vector<std::string>> get_unacknowledged_alarms();

  //预约相关操作
  bool add_reservation(const std::string &place_id, int user_id,
                       const std::string &purpose,
                       const std::string &start_time,
                       const std::string &end_time,
                       const std::string &status = "pending_teacher");

  bool update_reservation_status(int reservation_id, const std::string &status,
                                 const std::string &place_id);

  std::vector<std::vector<std::string>>
  get_reservations_by_place(const std::string &place_id);

  std::vector<std::vector<std::string>> get_all_reservations();

  bool check_reservation_conflict(const std::string &equipment_id,
                                  const std::string &start_time,
                                  const std::string &end_time);

  // 获取所有场所列表
  std::vector<std::vector<std::string>> get_all_places();

  // 根据场所ID获取设备列表
  std::vector<std::string>
  get_equipment_ids_by_place(const std::string &place_id);

  // 修改：冲突检测改为场所级别
  bool check_place_reservation_conflict(const std::string &place_id,
                                        const std::string &start_time,
                                        const std::string &end_time);

  // 查询操作
  std::vector<std::vector<std::string>> execute_query(const std::string &query);
  bool execute_update(const std::string &query);

  // 工具函数
  std::string get_last_error() const;

  // 根据用户名查询用户信息
  bool get_user_info(const std::string &username, std::string &password_hash,
                     std::string &role, int &user_id);

  // 获取某老师的所有学生ID
  std::vector<int> get_students_of_teacher(int teacher_id);

  // 判断老师是否是某学生的导师
  bool is_teacher_of_student(int teacher_id, int student_id);

  // 根据用户角色获取预约记录（用于查询）
  std::vector<std::vector<std::string>>
  get_reservations_for_user(int user_id, const std::string &role);

  // 根据场所ID和用户信息获取预约记录（角色过滤）
  std::vector<std::vector<std::string>>
  get_reservations_by_place_for_user(const std::string &place_id, int user_id,
                                     const std::string &role);

private:
  bool initialize_tables(); // 初始化数据库表

  MYSQL *mysql_conn_;
  std::string host_;
  std::string user_;
  std::string password_;
  std::string database_;
  int port_;
};
