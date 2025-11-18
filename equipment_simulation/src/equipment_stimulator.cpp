#include "equipment_stimulator.h"
#include "sys/socket.h"
#include <unistd.h>

std::unique_ptr<EquipmentStimulator>
EquipmentStimulator::create(Equipment &equipment, const std::string &server_ip,
                            std::uint16_t server_port) {
  // 1.创建socket
  int socket_fd = Socket::create_socket();
  if (socket_fd < 0) {
    std::cerr << "创建socket失败" << std::endl;
    return nullptr;
  }

  // 2.设置非阻塞
  if (!Socket::set_nonblock(socket_fd)) {
    close(socket_fd);
    return nullptr;
  }

  // 3.连接服务器
  if (!Socket::connect_to_socket(socket_fd, server_ip, server_port)) {
    std::cerr << "连接服务器失败: " << server_ip << ":" << server_port
              << std::endl;
    close(socket_fd);
    return nullptr;
  }
  std::cout << "设备模拟器创建成功: " << equipment.get_equipment_id()
            << ", 连接: " << server_ip << ":" << server_port << std::endl;

  // 4.创建EquipmentStimulator实例
  // 使用new调用私有构造函数，然后包装在unique_ptr中
  return std::unique_ptr<EquipmentStimulator>(
      new EquipmentStimulator(equipment, socket_fd));
}

//私有构造函数
EquipmentStimulator::EquipmentStimulator(Equipment &equipment, int socket_fd)
    : equipment_(equipment), socket_fd_(socket_fd) {
  std::cout << "EquipmentStimulator构造完成: " << equipment_.get_equipment_id()
            << std::endl;
}

//析构函数
EquipmentStimulator::~EquipmentStimulator() {
  if (socket_fd_ != -1) {
    std::cout << "关闭连接：" << equipment_.get_equipment_id() << std::endl;
    close(socket_fd_);
    socket_fd_ = -1;
  }
}

bool EquipmentStimulator::send_message_to_server(std::vector<char> &message) {
  if (socket_fd_ == -1) {
    std::cerr << "连接未建立，无法发送消息" << std::endl;
    return false;
  }
  ssize_t send_bytes = 0;
  ssize_t message_len = static_cast<ssize_t>(message.size());
  while (send_bytes < message_len) {
    ssize_t n_bytes = send(socket_fd_, message.data() + send_bytes,
                           message_len - send_bytes, 0);
    if (n_bytes < 0) {
      if (errno == EINTR) {
        //被信号中断
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        //缓冲区满，短暂等待后重试
        usleep(1000);
        continue;
      }
      std::perror("发送消息失败"); //真正出错
      return false;
    }
    send_bytes += n_bytes;
  }
  return true;
}
void EquipmentStimulator::send_registration() {
  std::vector<char> registration_message =
      ProtocolParser::build_register_message(equipment_.get_equipment_id(),
                                             equipment_.get_location(),
                                             equipment_.get_equipment_type());
  if (!send_message_to_server(registration_message)) {
    std::cout << "send registration message failed "
              << equipment_.get_equipment_id() << std::endl;
  }
  std::cout << "send registration message success "
            << equipment_.get_equipment_id() << std::endl;
}

void EquipmentStimulator::send_heartbeat() {
  std::vector<char> heartbeat_message =
      ProtocolParser::build_heartbeat_message(equipment_.get_equipment_id());
  if (!send_message_to_server(heartbeat_message)) {
    std::cout << "send heartbeat message failed..." << std::endl;
  }
}

void EquipmentStimulator::send_status_update() {
  std::vector<char> status_update_message =
      ProtocolParser::build_status_update_message(
          equipment_.get_equipment_id(), equipment_.get_status(),
          equipment_.get_power_state(), "45");
  if (!send_message_to_server(status_update_message)) {
    std::cout << "send status_update message failed..." << std::endl;
  }
}

void handle_server_command() {}