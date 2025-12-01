#include "equipment_management_server.h"
#include <iostream>
#include <system_error>
#include <unistd.h>
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
  std::cout << "服务器运行中，按 ctrl+c 停止..." << std::endl;

  // 等待用户按键
  std::cin.get(); // 这会阻塞直到用户按ctrl+c

  std::cout << "服务器正常退出" << std::endl;
  return 0;
}