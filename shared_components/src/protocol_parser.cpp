#include "protocol_parser.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
// 简单协议格式：类型|设备ID|数据
// 示例：
// 设备注册: "1|projector_101|classroom_101"
// 状态上报: "2|projector_101|online|on|45"
// 控制指令: "3|projector_101|turn_on"
// 心跳: "4|projector_101|"

// ============ 基础消息操作实现 ============

std::vector<char> ProtocolParser::pack_message(const std::string &body) {
  uint32_t body_len = body.length();
  uint32_t net_len = htonl(body_len); // 转换为网络字节序

  std::vector<char> packed_message(4 + body_len);

  // 写入长度头
  memcpy(packed_message.data(), &net_len, 4);
  // 写入消息体
  memcpy(packed_message.data() + 4, body.data(), body_len);

  return packed_message;
}

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
  if (type_num < 1 || type_num > 103) {
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

std::vector<std::string> ProtocolParser::split_string(const std::string &str,
                                                      char delimiter) {
  std::vector<std::string> tokens;
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

// ============ 私有工具函数 ============

std::string
ProtocolParser::build_message_body(MessageType type,
                                   const std::string &equipment_id,
                                   const std::vector<std::string> &fields) {
  std::string body =
      std::to_string(static_cast<int>(type)) + "|" + equipment_id;

  for (const auto &field : fields) {
    body += "|" + field;
  }

  return body;
}

// ============ 设备上线相关消息实现 ============

std::vector<char>
ProtocolParser::build_online_message(const std::string &equipment_id,
                                     const std::string &location,
                                     const std::string &equipment_type) {
  std::string body = build_message_body(EQUIPMENT_ONLINE, equipment_id,
                                        {location, equipment_type});
  return pack_message(body);
}

std::vector<char> ProtocolParser::build_online_response(bool success) {
  std::string body = build_message_body(ONLINE_RESPONSE, "response",
                                        {success ? "success" : "fail"});
  return pack_message(body);
}

// ============ 登录消息实现 ============

std::vector<char>
ProtocolParser::buildQtLoginResponseMessage(bool success,
                                            const std::string &message) {
  std::vector<std::string> fields;
  fields.push_back(success ? "success" : "fail");
  if (!message.empty()) {
    fields.push_back(message);
  }
  // 注意：这里设备ID为空字符串，使用专门的消息构建函数
  std::string body = build_message_body(QT_LOGIN_RESPONSE, "", fields);
  return pack_message(body);
}

// ============ 状态相关消息实现 ============

std::vector<char> ProtocolParser::build_status_update_message(
    const std::string &equipment_id, const std::string &status,
    const std::string &power_state, const std::string &more_data) {
  std::vector<std::string> fields = {status, power_state};
  if (!more_data.empty()) {
    fields.push_back(more_data);
  }

  std::string body = build_message_body(STATUS_UPDATE, equipment_id, fields);
  return pack_message(body);
}

std::vector<char>
ProtocolParser::build_status_query(const std::string &equipment_id) {
  std::string body = build_message_body(STATUS_QUERY, equipment_id);
  return pack_message(body);
}

std::vector<char>
ProtocolParser::build_status_response(const std::string &equipment_id,
                                      const std::string &status,
                                      const std::string &power_state) {
  std::string body =
      build_message_body(STATUS_RESPONSE, equipment_id, {status, power_state});
  return pack_message(body);
}

// ============ 控制相关消息实现 ============

std::vector<char>
ProtocolParser::build_control_command(const std::string &equipment_id,
                                      ControlCommandType command_type,
                                      const std::string &parameters) {
  std::vector<std::string> fields = {
      std::to_string(static_cast<int>(command_type))};
  if (!parameters.empty()) {
    fields.push_back(parameters);
  }

  std::string body = build_message_body(CONTROL_COMMAND, equipment_id, fields);
  return pack_message(body);
}

std::vector<char> ProtocolParser::build_control_response(
    const std::string &equipment_id, bool success, const std::string &message) {
  std::string body = build_message_body(
      CONTROL_RESPONSE, equipment_id, {success ? "success" : "fail", message});
  return pack_message(body);
}

// ============ 心跳相关消息实现 ============

std::vector<char>
ProtocolParser::build_heartbeat_message(const std::string &equipment_id) {
  std::string body = build_message_body(HEARTBEAT, equipment_id);
  return pack_message(body);
}

std::vector<char> ProtocolParser::build_heartbeat_response() {
  std::string body = build_message_body(HEARTBEAT_RESPONSE, "pong");
  return pack_message(body);
}

// ============ 预约系统消息实现 ============

std::vector<char>
ProtocolParser::build_reservation_response(bool success,
                                           const std::string &message) {
  std::string body = build_message_body(
      RESERVATION_APPLY, "response", {success ? "success" : "fail", message});
  return pack_message(body);
}

std::vector<char>
ProtocolParser::build_reservation_query_response(bool success,
                                                 const std::string &data) {
  std::string body = build_message_body(RESERVATION_QUERY, "response",
                                        {success ? "success" : "fail", data});
  return pack_message(body);
}

std::vector<char>
ProtocolParser::build_reservation_approve_response(bool success,
                                                   const std::string &message) {
  std::string body = build_message_body(
      RESERVATION_APPROVE, "response", {success ? "success" : "fail", message});
  return pack_message(body);
}
