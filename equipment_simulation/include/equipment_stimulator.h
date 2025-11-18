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
  //简单工厂模式:静态创建方法
  static std::unique_ptr<EquipmentStimulator>
  create(Equipment &equipment, const std::string &server_ip,
         std::uint16_t server_port);

  // //移动构造函数和移动赋值运算符
  // EquipmentStimulator(EquipmentStimulator &&other) noexcept;
  // EquipmentStimulator &operator=(EquipmentStimulator &&other) noexcept;

  //删除拷贝构造
  EquipmentStimulator(const EquipmentStimulator &) = delete;
  EquipmentStimulator &operator=(const EquipmentStimulator &) = delete;

  //析构函数自动清理资源
  ~EquipmentStimulator();

  //核心设备模拟功能

  void send_registration();

  void send_heartbeat();

  void send_status_update();

  void handle_server_commands();

  //状态查询
  bool is_connected() const { return socket_fd_ != -1; }
  bool is_running() const { return is_running_; }
  const std::string &get_equipment_id() {
    return equipment_.get_equipment_id();
  }
  const Equipment &get_equipment() const { return equipment_; };

  //控制接口
  void stop() { is_running_ = false; }
  void stimulate_power_on();
  void stimulate_power_off();

private:
  //私有构造函数，只能通过静态工厂方法创建
  EquipmentStimulator(Equipment &equipment, int socket_fd);

  //内部辅助方法
  bool send_message_to_server(std::vector<char> &message);
  void handle_control_command(const std::string &command);
  //成员变量
  Equipment equipment_;
  int socket_fd_{-1};
  // MessageBuffer read_buffer_;
  bool is_running_{true};
};