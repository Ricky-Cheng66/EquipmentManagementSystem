#pragma once
#include <string>
#include <vector>

class MessageBuffer {

public:
  MessageBuffer();

  // 追加接收到的数据
  void append_data(const char *data, size_t len);

  // 尝试从缓冲区提取完整消息
  // 返回提取到的消息数量，messages包含完整消息列表
  size_t extract_messages(std::vector<std::string> &messages);

  // 获取缓冲区当前数据量
  size_t data_size() const;

  // 清空缓冲区
  void clear();

  // 检查缓冲区是否过大（防止内存耗尽）
  bool is_too_large() const;

private:
  // 解析消息头获取消息长度
  bool parse_message_length(uint32_t &msg_len) const;

  std::vector<char> buffer_;
  static const size_t INITIAL_BUFFER_SIZE = 1024;
  static const size_t MAX_BUFFER_SIZE = 64 * 1024; // 64KB
};
