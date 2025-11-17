#include "equipment.h"
#include "message_buffer.h"
#include "protocol_parser.h"
#include "socket.h"
#include <memory>
#include <string>
#include <unordered_map>
class EquipmentStimulator {

public:
  EquipmentStimulator(Equipment &equipment);

  bool connect_to_server(std::string ip_address, std::uint16_t port);

  bool send_message_to_server(std::vector<char> registration_message);

  void send_registration();

  void send_heartbeat();

  void send_status_update();

  void handle_server_command();

private:
  std::unique_ptr<std::pair<int, Equipment>> stimulator_ptr_;
};