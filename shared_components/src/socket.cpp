#include "socket.h"

#include <arpa/inet.h>
#include <cstring>
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
bool Socket::connect_to_socket(int fd, std::string address, std::uint16_t port,
                               int timeout_ms) {
  std::cout << "DEBUG Socket::connect_to_socket: 开始连接 " << address << ":"
            << port << std::endl;
  // 1. 先验证 fd 是非阻塞的（可选，若调用者已确保可省略）
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    std::cerr << "fcntl get flags failed: " << strerror(errno) << std::endl;
    return false;
  }
  if (!(flags & O_NONBLOCK)) {
    std::cerr << "Error: fd is not non-blocking!" << std::endl;
    return false;
  }

  // 2. 初始化目标地址结构
  struct sockaddr_in destination {};
  destination.sin_family = AF_INET;
  destination.sin_port = htons(port);
  if (inet_pton(AF_INET, address.c_str(), &destination.sin_addr) <= 0) {
    std::cerr << "Invalid IPv4 address: " << address << std::endl;
    return false;
  }
  socklen_t dest_len = sizeof(destination);

  // 3. 调用非阻塞 connect
  int ret =
      connect(fd, reinterpret_cast<struct sockaddr *>(&destination), dest_len);
  if (ret == 0) {
    // 极少数情况：本地连接/快速连接，立即成功
    std::cout << "Connect success immediately" << std::endl;
    return true;
  }

  // 4. 处理 connect 返回 -1 的情况（核心错误判断）
  if (errno != EINPROGRESS) {
    // 不是 "连接正在进行"，而是真错误
    std::cerr << "Connect failed (not EINPROGRESS): " << strerror(errno)
              << std::endl;
    return false;
  }

  // 5. 到这里说明是 EINPROGRESS，用 select 等待连接结果
  fd_set write_fds; // 监听可写事件
  FD_ZERO(&write_fds);
  FD_SET(fd, &write_fds);

  // 设置超时（timeout_ms 毫秒）
  struct timeval timeout {};
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  // 等待 fd 可写或超时
  ret = select(fd + 1, nullptr, &write_fds, nullptr, &timeout);
  if (ret == -1) {
    // select 自身出错
    std::cerr << "select failed: " << strerror(errno) << std::endl;
    return false;
  } else if (ret == 0) {
    // 超时
    std::cerr << "Connect timeout (wait " << timeout_ms << "ms)" << std::endl;
    return false;
  }

  // 6. select 返回 >0，检查 fd 是否在可写集合中（理论上一定在，但保险起见）
  if (!FD_ISSET(fd, &write_fds)) {
    std::cerr << "select returned but fd is not writable (unexpected)"
              << std::endl;
    return false;
  }

  // 7. 关键：通过 getsockopt 获取实际连接状态（避免假可写）
  int so_error;
  socklen_t error_len = sizeof(so_error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &error_len) == -1) {
    std::cerr << "getsockopt failed: " << strerror(errno) << std::endl;
    return false;
  }

  // 8. 检查 SO_ERROR 的值（0 = 成功，非 0 = 具体错误）
  if (so_error != 0) {
    std::cerr << "Connect failed (SO_ERROR): " << strerror(so_error)
              << std::endl;
    return false;
  }

  // 9. 所有检查通过，连接成功
  std::cout << "Connect success (non-blocking) to " << address << ":" << port
            << std::endl;
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