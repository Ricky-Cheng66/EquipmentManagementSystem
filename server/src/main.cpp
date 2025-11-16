#include "../include/equipment_management_server.h"
#include <iostream>
#include <system_error>
int main() {
  //初始化Server
  EquipmentManagementServer server{};
  if (!server.init(9000)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "server init failed..." << ec.message() << std::endl;
    return -1;
  }
  //启动Server
  if (!server.start()) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "server start failed... " << ec.message() << std::endl;
    return -1;
  }
  return 0;
}