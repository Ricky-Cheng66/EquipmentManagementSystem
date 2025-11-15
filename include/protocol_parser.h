#pragma once
#include <string>
#include <vector>

class ProtocolParser {
public:
  // 消息类型
  enum MessageType {
    UNKNOWN_TYPE = 0,
    EQUIPMENT_REGISTER = 1, // 设备注册
    STATUS_UPDATE = 2,      // 状态上报
    CONTROL_CMD = 3,        // 控制指令
    HEARTBEAT = 4           // 心跳
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

  // 构建注册响应
  static std::vector<char> build_register_response(bool success);

  // 构建心跳响应
  static std::string build_heartbeat_response();

  // 分割字符串
  static std::vector<std::string> split_string(const std::string &str,
                                               char delimiter);
};
