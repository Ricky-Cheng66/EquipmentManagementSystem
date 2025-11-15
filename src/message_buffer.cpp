#include "../include/message_buffer.h"
#include <arpa/inet.h>
#include <cstring>

MessageBuffer::MessageBuffer() { buffer_.reserve(INITIAL_BUFFER_SIZE); }

void MessageBuffer::append_data(const char *data, size_t len) {
  if (buffer_.size() + len > MAX_BUFFER_SIZE) {
    // 防止缓冲区过大，可以在这里处理或抛出异常
    return;
  }

  size_t old_size = buffer_.size();
  buffer_.resize(old_size + len);
  memcpy(buffer_.data() + old_size, data, len);
}

size_t MessageBuffer::extract_messages(std::vector<std::string> &messages) {
  size_t extracted_count = 0;

  while (true) {
    // 检查是否有足够数据读取消息头
    if (buffer_.size() < 4) {
      break; // 连消息头都不完整，等待更多数据
    }

    // 解析消息长度
    uint32_t msg_len;
    if (!parse_message_length(msg_len)) {
      // 消息头解析失败，清空缓冲区（协议错误）
      clear();
      break;
    }

    // 检查是否有完整的消息体
    if (buffer_.size() < 4 + msg_len) {
      break; // 消息体不完整，等待更多数据
    }

    // 提取完整消息
    std::string message(buffer_.data() + 4, msg_len);
    messages.push_back(message);
    extracted_count++;

    // 从缓冲区移除已处理的消息
    size_t remaining = buffer_.size() - (4 + msg_len);
    if (remaining > 0) {
      // 移动剩余数据到缓冲区开头
      memmove(buffer_.data(), buffer_.data() + 4 + msg_len, remaining);
    }
    buffer_.resize(remaining);
  }

  return extracted_count;
}

bool MessageBuffer::parse_message_length(uint32_t &msg_len) const {
  if (buffer_.size() < 4) {
    return false;
  }

  // 从网络字节序转换为主机字节序
  uint32_t net_len;
  memcpy(&net_len, buffer_.data(), 4);
  msg_len = ntohl(net_len);

  // 简单的长度校验（防止恶意数据）
  if (msg_len > MAX_BUFFER_SIZE) {
    return false;
  }

  return true;
}

size_t MessageBuffer::data_size() const { return buffer_.size(); }

void MessageBuffer::clear() { buffer_.clear(); }

bool MessageBuffer::is_too_large() const {
  return buffer_.size() > MAX_BUFFER_SIZE;
}