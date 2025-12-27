#pragma once
#include <string>
#include <vector>

class ProtocolParser {
public:
  // ============ 消息类型枚举 ============
  enum MessageType {
    UNKNOWN_TYPE = 0,

    // 设备上线相关
    EQUIPMENT_ONLINE = 1, // 设备→服务器：设备上线
    ONLINE_RESPONSE = 2,  // 服务器→设备：上线响应

    // 状态相关
    STATUS_UPDATE = 3,   // 设备→服务器：状态主动上报
    STATUS_QUERY = 4,    // 服务器→设备：状态查询
    STATUS_RESPONSE = 5, // 设备→服务器：状态查询响应

    // 控制相关
    CONTROL_COMMAND = 6,  // 服务器→设备：控制命令
    CONTROL_RESPONSE = 7, // 设备→服务器：控制响应

    // 心跳相关
    HEARTBEAT = 8,          // 设备→服务器：心跳
    HEARTBEAT_RESPONSE = 9, // 服务器→设备：心跳响应

    // Qt客户端接口
    QT_CONTROL_REQUEST = 10,  // Qt→服务器：控制请求
    QT_CONTROL_RESPONSE = 11, // 服务器→Qt：控制响应
    QT_STATUS_QUERY = 12,     // Qt→服务器：状态查询
    QT_STATUS_RESPONSE = 13,  // 服务器→Qt：状态响应

    // 预约系统
    RESERVATION_APPLY = 14,
    RESERVATION_QUERY = 15,
    RESERVATION_APPROVE = 16,

    // ===== Qt客户端专用消息 =====
    QT_CLIENT_LOGIN = 100,         // Qt客户端 -> 服务器：登录请求
    QT_LOGIN_RESPONSE = 101,       // 服务器 -> Qt客户端：登录响应
    QT_EQUIPMENT_LIST_QUERY = 102, // Qt客户端 -> 服务器：查询设备列表
    QT_EQUIPMENT_LIST_RESPONSE = 103, // 服务器 -> Qt客户端：返回设备列表
    QT_RESERVATION_APPLY_RESPONSE = 104, // 服务器 -> Qt客户端：预约申请响应
    QT_RESERVATION_QUERY_RESPONSE = 105, // 服务器 -> Qt客户端：预约查询响应
    QT_RESERVATION_APPROVE_RESPONSE = 106 // 服务器 -> Qt客户端：预约审批响应
  };

  // ============ 客户端类型枚举 ============
  enum ClientType {
    CLIENT_UNKNOWN = 0,
    CLIENT_EQUIPMENT = 1, // 设备模拟端
    CLIENT_QT_CLIENT = 2  // Qt客户端
  };

  // ============ 控制命令类型枚举 ============
  enum ControlCommandType {
    TURN_ON = 1,
    TURN_OFF = 2,
    RESTART = 3,
    ADJUST_SETTINGS = 4,
    GET_STATUS = 5
  };

  // ============ 解析结果结构 ============
  struct ParseResult {
    ClientType client_type;
    MessageType type;
    std::string equipment_id;
    std::string payload;
    bool success;
  };

  // ============ 基础消息操作 ============
  static std::vector<char> pack_message(const std::string &body);
  static ParseResult parse_message(const std::string &data);
  static std::vector<std::string> split_string(const std::string &str,
                                               char delimiter);
  // Qt客户端登录相关
  static std::vector<char>
  build_qt_login_message(ProtocolParser::ClientType client_type,
                         const std::string &username,
                         const std::string &password);
  static std::vector<char>
  build_qt_login_response_message(ProtocolParser::ClientType client_type,
                                  bool success,
                                  const std::string &message = "");
  static std::vector<char>
  build_qt_equipment_list_query(ProtocolParser::ClientType client_type);

  // ============ 设备上线相关消息构建 ============
  static std::vector<char>
  build_online_message(ClientType client_type, const std::string &equipment_id,
                       const std::string &location,
                       const std::string &equipment_type);
  static std::vector<char> build_online_response(ClientType client_type,
                                                 bool success);

  // ============登录消息构建 ============
  static std::vector<char>
  buildQtLoginResponseMessage(ClientType client_type, bool success,
                              const std::string &message = "");

  // ============ 状态相关消息构建 ============
  static std::vector<char> build_status_update_message(
      ClientType client_type, const std::string &equipment_id,
      const std::string &status, const std::string &power_state,
      const std::string &more_data = "");
  static std::vector<char> build_status_query(ClientType client_type,
                                              const std::string &equipment_id);
  static std::vector<char>
  build_status_response(ClientType client_type, const std::string &equipment_id,
                        const std::string &status,
                        const std::string &power_state);

  // ============ 预约系统消息构建 ============
  static std::vector<char>
  build_reservation_message(ClientType client_type,
                            const std::string &equipment_id,
                            const std::string &payload);
  static std::vector<char>
  build_reservation_query(ClientType client_type,
                          const std::string &equipment_id);
  static std::vector<char>
  build_reservation_approve(ClientType client_type, const std::string &admin_id,
                            const std::string &payload);

  // ============ 控制相关消息构建 ============
  static std::vector<char>
  build_control_command(ClientType client_type, const std::string &equipment_id,
                        ControlCommandType command_type,
                        const std::string &parameters = "");

  static std::vector<char> build_control_command_to_server(
      ClientType client_type, const std::string &equipment_id,
      ControlCommandType command_type, const std::string &parameters = "");
  static std::vector<char>
  build_control_response(ClientType client_type,
                         const std::string &equipment_id, bool success,
                         const std::string &parameters);

  // ============ 心跳相关消息构建 ============
  static std::vector<char>
  build_heartbeat_message(ClientType client_type,
                          const std::string &equipment_id);
  static std::vector<char> build_heartbeat_response(ClientType client_type);

  // ============ 预约系统消息构建 ============
  static std::vector<char>
  build_reservation_response(ClientType client_type, bool success,
                             const std::string &message);
  static std::vector<char>
  build_reservation_query_response(ClientType client_type, bool success,
                                   const std::string &data);
  static std::vector<char>
  build_reservation_approve_response(ClientType client_type, bool success,
                                     const std::string &message);

private:
  // 工具函数 - 构建基础消息体
  static std::string
  build_message_body(ClientType client_type, MessageType type,
                     const std::string &equipment_id,
                     const std::vector<std::string> &fields = {});
};