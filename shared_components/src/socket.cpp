#include "socket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <system_error>

int Socket::create_socket() {
  // create server_fd
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "socket failed..." << ec.message() << std::endl;
    return -1;
  }
  return fd;
}
bool Socket::connect_to_socket(int fd, std::string address,
                               std::uint16_t port) {
  int source_fd = fd;
  struct sockaddr_in destination {};
  destination.sin_family = AF_INET;
  destination.sin_port = htons(port);
  inet_pton(AF_INET, address.c_str(), &destination.sin_addr);
  socklen_t destination_len = sizeof(destination);
  if (connect(source_fd, (struct sockaddr *)&destination, destination_len) <
      0) {
    std::cout << "connect failed..." << std::endl;
    return false;
  }
  return true;
}

bool Socket::bind_server_socket(int fd, int port) {
  struct sockaddr_in server_addr_in {};
  server_addr_in.sin_addr.s_addr = INADDR_ANY;
  server_addr_in.sin_family = AF_INET;
  server_addr_in.sin_port = htons(port);
  socklen_t server_len = sizeof(server_addr_in);
  if (bind(fd, (sockaddr *)&server_addr_in, server_len) < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "bind failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}
int Socket::accept_socket(int server_fd) {
  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "accept failed..." << ec.message() << std::endl;
    return -1;
  }
  return client_fd;
}
bool Socket::listen_socket(int fd) {
  if (listen(fd, max_clients_) < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "listen failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}
bool Socket::set_socket_option(int fd) {
  // set REUSADDR
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "setsockopt failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}

bool Socket::set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "F_GETFL failed..." << ec.message() << std::endl;
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "F_SETFL failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}