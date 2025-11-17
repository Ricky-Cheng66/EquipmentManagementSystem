#include "epoll.h"
#include "socket.h"

#include <iostream>
#include <sys/epoll.h>
#include <system_error>
#include <unistd.h>
Epoll::~Epoll() {
  if (epfd_ != -1) {
    close(epfd_);
    epfd_ = -1;
  }
}
bool Epoll::initialize() {
  if (epfd_ != -1) {
    std::cerr << "Epoll already initialized..." << std::endl;
    return true; //已经初始化过了
  }
  epfd_ = epoll_create1(0);
  if (epfd_ < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_create1 failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}

bool Epoll::add_epoll(int fd, uint32_t event) {
  std::lock_guard<std::mutex> lock(epoll_mutex_);
  struct epoll_event ev {};
  ev.data.fd = fd;
  ev.events = event;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_ctl add failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}
bool Epoll::delete_epoll(int fd) {
  std::lock_guard<std::mutex> lock(epoll_mutex_);
  // 检查文件描述符是否有效
  if (fd <= 0) {
    return false;
  }
  if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
    // 忽略常见的无害错误
    if (errno == ENOENT || // No such file or directory
        errno == EBADF) {  // Bad file descriptor
      return true;
    }
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_ctl del failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}
bool Epoll::modify_epoll(int fd, uint32_t event) {
  std::lock_guard<std::mutex> lock(epoll_mutex_);
  // 检查文件描述符是否有效
  if (fd <= 0) {
    return false;
  }
  struct epoll_event ev {};
  ev.events = event;
  ev.data.fd = fd;
  std::cout << "DEBUG modify_epoll: fd=" << fd << ", events=0x" << std::hex
            << event << std::dec << std::endl;
  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_ctl mod failed..." << ec.message() << std::endl;
    return false;
  }
  std::cout << "DEBUG modify_epoll: success" << std::endl;
  return true;
}
int Epoll::wait_events(struct epoll_event *evs, int timeout) {
  if (epfd_ < 0) {
    return -1;
  }
  timeout = -1;
  int nfds = epoll_wait(epfd_, evs, max_events_, timeout);
  if (nfds < 0) {
    if (errno == EINTR) {
      //被信号中断
      std::cerr << "epoll_wait 被信号中断" << std::endl;
      return -1;
    }
    std::cerr << "epoll_wait other errors" << std::endl;
    return -1;
  }
  return nfds;
}