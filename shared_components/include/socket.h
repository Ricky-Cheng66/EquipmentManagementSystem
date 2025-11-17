#pragma once
#include <iostream>
class Socket {
public:
  Socket() = default;
  ~Socket() = default;
  static int create_socket();
  static bool connect_to_socket(int fd, std::string address,
                                std::uint16_t port);
  bool bind_server_socket(int fd, int port);
  int accept_socket(int server_fd);
  bool listen_socket(int fd);
  static bool set_socket_option(int fd);
  static bool set_nonblock(int fd);

private:
  int max_clients_ = 1024;
};