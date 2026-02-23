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
    if (parts.size() < 3) {
        return result;
    }
    //解析客户端类型
    int client_type_num = std::stoi(parts[0]);
    result.client_type = static_cast<ClientType>(client_type_num);

    // 解析消息类型
    int type_num = std::stoi(parts[1]);
    if (type_num < 1 || type_num > 200) {
        return result;
    }
    result.type = static_cast<MessageType>(type_num);

    // 设备ID
    result.equipment_id = parts[2];

    //  payload
    if (parts.size() > 3) {
        for (size_t i = 3; i < parts.size(); ++i) {
            if (i > 3)
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

// ============ qt客户端登录相关 ========
// Qt客户端登录消息
std::vector<char>
ProtocolParser::build_qt_login_message(ProtocolParser::ClientType client_type,
                                       const std::string &username,
                                       const std::string &password) {
    std::string body = std::to_string(client_type) + "|" +
                       std::to_string(static_cast<int>(QT_CLIENT_LOGIN)) + "|" +
                       "qt_client" + "|" + // 固定设备ID，标识为Qt客户端
                       username + "|" + password;
    return pack_message(body);
}

// Qt客户端登录响应
std::vector<char> ProtocolParser::build_qt_login_response_message(
    ProtocolParser::ClientType client_type, bool success,
    const std::string &message) {
    std::vector<std::string> fields;
    fields.push_back(success ? "success" : "fail");
    if (!message.empty()) {
        fields.push_back(message);
    }
    std::string body =
        build_message_body(client_type, QT_LOGIN_RESPONSE, "", fields);
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_qt_equipment_list_query(
    ProtocolParser::ClientType client_type) {
    std::string body = std::to_string(client_type) + "|" +
                       std::to_string(static_cast<int>(QT_EQUIPMENT_LIST_QUERY)) +
                       "||";
    return pack_message(body);
}

// ============ Qt客户端心跳消息实现 ============

std::vector<char> ProtocolParser::build_qt_heartbeat_message(
    ClientType client_type, const std::string &client_identifier) {
    // payload为空
    return pack_message(
        build_message_body(client_type, QT_HEARTBEAT, client_identifier, {}));
}

std::vector<char> ProtocolParser::build_qt_heartbeat_response(
    ClientType client_type, const std::string &client_identifier,
    const std::string &timestamp) {
    return pack_message(build_message_body(client_type, QT_HEARTBEAT_RESPONSE,
                                           client_identifier, {timestamp}));
}

std::vector<char>
ProtocolParser::build_set_threshold_message(ClientType client_type,
                                            const std::string &equipment_id,
                                            float threshold_value) {
    // payload: "equipment_id|threshold_value"
    std::string payload = equipment_id + "|" + std::to_string(threshold_value);
    return pack_message(build_message_body(client_type, QT_SET_THRESHOLD,
                                           equipment_id, {payload}));
}

std::vector<char> ProtocolParser::build_set_threshold_response(
    ClientType client_type, bool success, const std::string &message) {
    return pack_message(
        build_message_body(client_type, QT_SET_THRESHOLD_RESPONSE, "response",
                           {success ? "success" : "fail", message}));
}

// ============ 私有工具函数 ============

std::string
ProtocolParser::build_message_body(ClientType client_type, MessageType type,
                                   const std::string &equipment_id,
                                   const std::vector<std::string> &fields) {
    std::string body = std::to_string(static_cast<int>(client_type)) + "|" +
                       std::to_string(static_cast<int>(type)) + "|" +
                       equipment_id;

    for (const auto &field : fields) {
        body += "|" + field;
    }

    return body;
}

// ============ 设备上线相关消息实现 ============

std::vector<char> ProtocolParser::build_online_message(
    ClientType client_type, const std::string &equipment_id,
    const std::string &location, const std::string &equipment_type) {
    std::string body = build_message_body(
        client_type, EQUIPMENT_ONLINE, equipment_id, {location, equipment_type});
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_online_response(ClientType client_type,
                                                        bool success) {
    std::string body =
        build_message_body(client_type, MessageType::ONLINE_RESPONSE, "response",
                           {success ? "success" : "fail"});
    return pack_message(body);
}

// ============ 登录消息实现 ============

std::vector<char> ProtocolParser::buildQtLoginResponseMessage(
    ClientType client_type, bool success, const std::string &message) {
    std::vector<std::string> fields;
    fields.push_back(success ? "success" : "fail");
    if (!message.empty()) {
        fields.push_back(message);
    }
    // 注意：这里设备ID为空字符串，使用专门的消息构建函数
    std::string body = build_message_body(
        client_type, MessageType::QT_LOGIN_RESPONSE, "", fields);
    return pack_message(body);
}

// ============ 状态相关消息实现 ============

std::vector<char> ProtocolParser::build_status_update_message(
    ClientType client_type, const std::string &equipment_id,
    const std::string &status, const std::string &power_state,
    const std::string &more_data) {
    std::vector<std::string> fields = {status, power_state};
    if (!more_data.empty()) {
        fields.push_back(more_data);
    }

    std::string body = build_message_body(client_type, MessageType::STATUS_UPDATE,
                                          equipment_id, fields);
    return pack_message(body);
}

std::vector<char>
ProtocolParser::build_status_query(ClientType client_type,
                                   const std::string &equipment_id) {
    std::string body =
        build_message_body(client_type, MessageType::STATUS_QUERY, equipment_id);
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_status_response(
    ClientType client_type, const std::string &equipment_id,
    const std::string &status, const std::string &power_state) {
    std::string body =
        build_message_body(client_type, MessageType::STATUS_RESPONSE,
                           equipment_id, {status, power_state});
    return pack_message(body);
}

// ============ 控制相关消息实现 ============

std::vector<char> ProtocolParser::build_control_command(
    ClientType client_type, const std::string &equipment_id,
    ControlCommandType command_type, const std::string &parameters) {
    std::vector<std::string> fields = {
                                       std::to_string(static_cast<int>(command_type))};
    if (!parameters.empty()) {
        fields.push_back(parameters);
    }

    std::string body = build_message_body(
        client_type, MessageType::CONTROL_COMMAND, equipment_id, fields);
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_control_command_to_server(
    ClientType client_type, const std::string &equipment_id,
    ControlCommandType command_type, const std::string &parameters) {
    std::vector<std::string> fields = {
                                       std::to_string(static_cast<int>(command_type))};
    if (!parameters.empty()) {
        fields.push_back(parameters);
    }

    std::string body = build_message_body(
        client_type, MessageType::QT_CONTROL_REQUEST, equipment_id, fields);
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_control_response(
    ClientType client_type, const std::string &equipment_id, bool success,
    const std::string &parameters) {
    std::string body = build_message_body(
        client_type, MessageType::CONTROL_RESPONSE, equipment_id, {parameters});
    return pack_message(body);
}

// ============ 心跳相关消息实现 ============

std::vector<char>
ProtocolParser::build_heartbeat_message(ClientType client_type,
                                        const std::string &equipment_id) {
    std::string body =
        build_message_body(client_type, MessageType::HEARTBEAT, equipment_id);
    return pack_message(body);
}

std::vector<char>
ProtocolParser::build_heartbeat_response(ClientType client_type) {
    std::string body =
        build_message_body(client_type, MessageType::HEARTBEAT_RESPONSE, "pong");
    return pack_message(body);
}

// ============ 预约系统消息实现 ============

std::vector<char>
ProtocolParser::build_reservation_response(ClientType client_type, bool success,
                                           const std::string &message) {
    std::string body =
        build_message_body(client_type, MessageType::RESERVATION_APPLY,
                           "response", {success ? "success" : "fail", message});
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_reservation_query_response(
    ClientType client_type, bool success, const std::string &data) {
    std::string body =
        build_message_body(client_type, MessageType::RESERVATION_QUERY,
                           "response", {success ? "success" : "fail", data});
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_reservation_approve_response(
    ClientType client_type, bool success, const std::string &message) {
    std::string body =
        build_message_body(client_type, MessageType::RESERVATION_APPROVE,
                           "response", {success ? "success" : "fail", message});
    return pack_message(body);
}

// ============ Qt端预约请求消息实现 ============

std::vector<char>
ProtocolParser::build_reservation_message(ClientType client_type,
                                          const std::string &place_id,
                                          const std::string &payload) {
    std::string body = build_message_body(
        client_type, MessageType::RESERVATION_APPLY, place_id, {payload});
    return pack_message(body);
}

std::vector<char>
ProtocolParser::build_reservation_query(ClientType client_type,
                                        const std::string &equipment_id) {
    std::string body = build_message_body(
        client_type, MessageType::RESERVATION_QUERY, equipment_id, {});
    return pack_message(body);
}

std::vector<char>
ProtocolParser::build_reservation_approve(ClientType client_type,
                                          const std::string &place_id,
                                          const std::string &payload) {
    std::string body = build_message_body(
        client_type, MessageType::RESERVATION_APPROVE, place_id, {payload});
    return pack_message(body);
}

std::vector<char> ProtocolParser::build_power_report_message(
    ClientType client_type, const std::string &equipment_id,
    const std::string &power_state, int power_value,
    const std::string &timestamp) {
    std::string payload =
        power_state + "|" + std::to_string(power_value) + "|" + timestamp;
    std::string body =
        build_message_body(client_type, POWER_REPORT, equipment_id, {payload});
    return pack_message(body);
}

// ============ 告警系统消息实现 ============

std::vector<char> ProtocolParser::build_alert_message(
    ClientType client_type, const std::string &equipment_id,
    const std::string &alarm_type, const std::string &severity,
    const std::string &message) {
    // payload格式: "alarm_type|severity|message"
    return pack_message(build_message_body(client_type, QT_ALERT_MESSAGE,
                                           equipment_id,
                                           {alarm_type, severity, message}));
}

std::vector<char>
ProtocolParser::build_alert_ack(ClientType client_type,
                                const std::string &equipment_id, int alarm_id) {
    return pack_message(build_message_body(
        client_type, QT_ALERT_ACK, equipment_id, {std::to_string(alarm_id)}));
}
