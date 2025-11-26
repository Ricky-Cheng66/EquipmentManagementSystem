#pragma once
#include <string>
#include <vector>

class ProtocolParser {
public:
  // 消息类型
  enum MessageType {

    UNKNOWN_TYPE = 0,

    // 设备注册相关
    EQUIPMENT_REGISTER = 1, // 设备→服务器：设备注册
    REGISTER_RESPONSE = 2,  // 服务器→设备：注册响应

    // 状态相关
    STATUS_UPDATE = 3,   // 设备→服务器：状态主动上报
    STATUS_QUERY = 4,    // 服务器→设备：状态查询
    STATUS_RESPONSE = 5, // 设备→服务器：状态查询响应

    // 控制相关
    CONTROL_COMMAND = 6,  // 服务器→设备：控制命令
    CONTROL_RESPONSE = 7, // 设备→服务器：控制响应

    // 心跳
    HEARTBEAT = 8,          // 设备→服务器：心跳
    HEARTBEAT_RESPONSE = 9, // 服务器→设备：心跳响应

    // 预留Qt客户端接口
    QT_CONTROL_REQUEST = 10,  // Qt→服务器：控制请求
    QT_CONTROL_RESPONSE = 11, // 服务器→Qt：控制响应
    QT_STATUS_QUERY = 12,     // Qt→服务器：状态查询
    QT_STATUS_RESPONSE = 13,  // 服务器→Qt：状态响应

    // 预约系统（保持原有）
    RESERVATION_APPLY = 14,
    RESERVATION_QUERY = 15,
    RESERVATION_APPROVE = 16
  };
  //控制命令类型
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
  static std::vector<char>
  build_control_command(const std::string &equipment_id,
                        ControlCommandType command_type,
                        const std::string &parameters);

  // 构建注册响应
  static std::vector<char> build_register_response(bool success);

  // 构建心跳响应
  static std::string build_heartbeat_response();

  // 构建状态查询（服务器→设备）
  std::vector<char> build_status_query(const std::string &equipment_id);

  //构建控制响应消息
  static std::vector<char>
  build_control_response(const std::string &equipment_id, bool success,
                         const std::string &message);

  // 构建状态响应（设备→服务器）
  std::vector<char> build_status_response(const std::string &equipment_id,
                                          const std::string &status,
                                          const std::string &power_state);

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
