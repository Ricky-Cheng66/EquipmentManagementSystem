#pragma once
#include <string>
#include <vector>

class ProtocolParser {
public:
  // 消息类型
  enum MessageType {
    UNKNOWN_TYPE = 0,
    EQUIPMENT_REGISTER = 1,  // 设备注册
    STATUS_UPDATE = 2,       // 状态上报
    CONTROL_CMD = 3,         // 控制指令
    HEARTBEAT = 4,           // 心跳
    RESERVATION_APPLY = 5,   //预约申请
    RESERVATION_QUERY = 6,   //预约查询
    RESERVATION_APPROVE = 7, //预约审批
    CONTROL_RESPONSE = 8     //控制响应
  };

  enum ControlCommandType {
    TURN_ON = 1,
    TURN_OFF = 2,
    RESTART = 3,
    ADJUST_SETTINGS = 4,
    GET_STATUS = 5
  };

  // 解析结果
  struct ParseResult {
    MessageType type;
    std::string equipment_id;
    std::string payload; // 简单字符串，用|分隔
    bool success;
  };

  // 打包消息：添加4字节长度头（网络字节序）
  static std::vector<char> pack_message(const std::string &body);

  // 解析设备消息
  static ParseResult parse_message(const std::string &data);

  // 构建控制消息
  static std::vector<char>
  build_control_message(const std::string &equipment_id,
                        const std::string &command);

  //构建注册信息
  static std::vector<char>
  build_register_message(const std::string &equipment_id,
                         const std::string &location,
                         const std::string &equipment_type);

  //构建心跳信息
  static std::vector<char>
  build_heartbeat_message(const std::string &equipment_id);
  //构建状态更新信息
  static std::vector<char> build_status_update_message(
      const std::string &equipment_id, const std::string status,
      const std::string power_state, const std::string more_data);

  //构建控制命令消息（服务器发送给设备）
  std::vector<char> build_control_command(const std::string &equipment_id,
                                          ControlCommandType command_type,
                                          const std::string &parameters);

  // 构建注册响应
  static std::vector<char> build_register_response(bool success);

  // 构建心跳响应
  static std::string build_heartbeat_response();

  //构建控制响应消息
  static std::vector<char>
  build_control_response(const std::string &equipment_id, bool success,
                         const std::string &message);

  // 分割字符串
  static std::vector<std::string> split_string(const std::string &str,
                                               char delimiter);

  static std::vector<char>
  build_reservation_response(bool success, const std::string &message);
  static std::vector<char>
  build_reservation_query_response(bool success, const std::string &data);

  static std::vector<char>
  build_reservation_approve_response(bool success, const std::string &message);
};
