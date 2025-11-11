#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

#include "../include/epoll.h"
#include "../include/equipment_management_server.h"
#include "../include/socket.h"
bool EquipmentManagementServer::init(int server_port) {
  // socket部分
  server_port_ = server_port;
  // create server_fd
  Socket server_socket{};
  server_fd_ = server_socket.create_server_socket();
  //设置地址重用
  if (!server_socket.set_socket_option(server_fd_)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "set_socket_option failed..." << ec.message() << std::endl;
  }
  // set listenFd nonblock
  server_socket.set_nonblock(server_fd_);
  // blind
  server_socket.bind_server_socket(server_fd_, server_port_);
  // listen
  server_socket.listen_socket(server_fd_);
  // create epfd and put listenFd into epoll
  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    return false;
  }
  // add_epoll
  ep.add_epoll(server_fd_, EPOLLIN | EPOLLET);
  return true;
}

bool EquipmentManagementServer::start() {
  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    return false;
  }

  int max_events = ep.get_epoll_max_events();
  struct epoll_event evs[max_events];

  while (1) {
    int nfds = ep.wait_events(evs, -1);
    std::cout << "DEBUG: epoll_wait returned " << nfds << " events"
              << std::endl;
    if (nfds < 0) {
      if (errno == EINTR) {
        //被信号中断
        std::cerr << "epoll_wait 被信号中断" << std::endl;
        continue;
      }
      std::cerr << "epoll_wait other errors" << std::endl;
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      std::cout << "DEBUG: Event " << i << ": fd=" << evs[i].data.fd
                << ", events=0x" << std::hex << evs[i].events << std::dec
                << std::endl;
      if (evs[i].data.fd == server_fd_) {
        // accept新连接
        Socket server_socket{};
        int client_fd = server_socket.accept_socket(evs[i].data.fd);
        if (client_fd < 0) {
          if (errno == EAGAIN || errno == EMFILE) {
            continue; // 资源暂时不可用
          }
          std::error_code ec(errno, std::system_category());
          std::cerr << "accept failed..." << ec.message() << std::endl;
          continue;
        }

        Socket client_socket{};
        client_socket.set_nonblock(client_fd);

        // 获取客户端地址
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getpeername(client_fd, (struct sockaddr *)&client_addr, &client_len);
        std::cout << "new client has connected..." << std::endl;
        // 注册到epoll（ET模式）
        ep.add_epoll(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);

      } else {
      }
    }
  }
  return true;
}