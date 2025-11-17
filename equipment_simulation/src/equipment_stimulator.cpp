#include "equipment_stimulator.h"
#include "sys/socket.h"
EquipmentStimulator::EquipmentStimulator(Equipment &equipment)
    : stimulator_ptr_(std::make_unique<std::pair<int, Equipment>>(
          Socket::create_socket(), equipment)) {
  Socket::set_nonblock(stimulator_ptr_->first);
}

bool EquipmentStimulator::connect_to_server(std::string ip_address,
                                            std::uint16_t port) {
  //服务器ip：192.168.198.129 端口：9090
  if (!Socket::connect_to_socket(stimulator_ptr_->first, ip_address, port)) {
    std::cout << "connect_to_server failed..." << std::endl;
    return false;
  }
  return true;
}

bool EquipmentStimulator::send_message_to_server(
    std::vector<char> registration_message) {
  ssize_t send_bytes = 0;
  ssize_t message_len = static_cast<ssize_t>(registration_message.size());
  while (send_bytes < message_len) {
    ssize_t n_bytes =
        send(stimulator_ptr_->first, registration_message.data() + send_bytes,
             message_len - send_bytes, 0);
    if (n_bytes < 0) {
      if (errno == EINTR || errno == EWOULDBLOCK) {
        //被信号中断或者缓冲区满
        continue;
      }
      std::perror("send"); //真正出错
      return false;
    }
    send_bytes += n_bytes;
  }
  return true;
}
void EquipmentStimulator::send_registration() {
  std::string registration_message_body{
      "1|" + stimulator_ptr_->second.get_equipment_id() + "|" +
      stimulator_ptr_->second.get_location() + "|" +
      stimulator_ptr_->second.get_equipment_type()};
  std::vector<char> registration_message =
      ProtocolParser::build_register_message(
          stimulator_ptr_->second.get_equipment_id(),
          stimulator_ptr_->second.get_location(),
          stimulator_ptr_->second.get_equipment_type());
  if (!send_message_to_server(registration_message)) {
    std::cout << "send_message_to_server failed..." << std::endl;
  }
}

void send_heartbeat() {}

void send_status_update() {}

void handle_server_command() {}