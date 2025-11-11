#pragma once
class Socket {
public:
  Socket() = default;
  ~Socket() = default;
  int create_server_socket();
  bool bind_server_socket(int fd, int port);
  int accept_socket(int server_fd);
  bool listen_socket(int fd);
  bool set_socket_option(int fd);
  bool set_nonblock(int fd);

private:
  int max_clients_ = 1024;
};