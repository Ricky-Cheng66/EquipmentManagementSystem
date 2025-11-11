#pragma once

#include "epoll.h"
#include <string>
#include <unordered_map>

class EquipmentManagementServer {
public:
  EquipmentManagementServer() = default;
  ~EquipmentManagementServer() = default;

  bool start();

  bool init(int server_port);

private:
  const int MAXCLIENTFDS = 1024;
  int server_fd_;
  std::string server_addr_;
  int server_port_;
};