#include "../include/protocol_parser.h"
#include <sstream>

// 简单协议格式：类型|设备ID|数据
// 示例：
// 设备注册: "1|projector_101|classroom_101"
// 状态上报: "2|projector_101|online|on|45"
// 控制指令: "3|projector_101|turn_on"
// 心跳: "4|projector_101|"

ProtocolParser::ParseResult
ProtocolParser::parse_message(const std::string &data) {
  ParseResult result;
  result.success = false;

  // 按|分割消息
  auto parts = split_string(data, '|');
  if (parts.size() < 2) {
    return result;
  }

  // 解析消息类型
  int type_num = std::stoi(parts[0]);
  if (type_num < 1 || type_num > 4) {
    return result;
  }
  result.type = static_cast<MessageType>(type_num);

  // 设备ID
  result.equipment_id = parts[1];

  //  payload是剩余部分
  if (parts.size() > 2) {
    for (size_t i = 2; i < parts.size(); ++i) {
      if (i > 2)
        result.payload += "|";
      result.payload += parts[i];
    }
  }

  result.success = true;
  return result;
}

// 构建控制消息: "3|设备ID|命令"
std::string
ProtocolParser::build_control_message(const std::string &equipment_id,
                                      const std::string &command) {
  return "3|" + equipment_id + "|" + command;
}

// 构建注册响应: "1|响应|success" 或 "1|响应|fail"
std::string ProtocolParser::build_register_response(bool success) {
  return "1|response|" + std::string(success ? "success" : "fail");
}

// 构建心跳响应: "4|pong"
std::string ProtocolParser::build_heartbeat_response() { return "4|pong"; }

// 分割字符串工具函数
std::vector<std::string> ProtocolParser::split_string(const std::string &str,
                                                      char delimiter) {
  std::vector<std::string> tokens;
  // 安全检查
  if (str.empty()) {
    return tokens;
  }
  std::string token;
  std::istringstream token_stream(str);

  while (std::getline(token_stream, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}