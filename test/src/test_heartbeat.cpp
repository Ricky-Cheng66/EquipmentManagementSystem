#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

std::atomic<bool> keep_running{true};

// 信号处理函数
void signal_handler(int signal) {
  std::cout << "\n接收到停止信号，准备退出..." << std::endl;
  keep_running = false;
}

int main() {
  std::cout << "=== 心跳机制验证测试 ===" << std::endl;

  // 注册信号处理
  std::signal(SIGINT, signal_handler);

  // 清理旧进程
  std::cout << "清理旧进程..." << std::endl;
  system("pkill -f EMS_server 2>/dev/null");
  system("pkill -f equipment_simulation 2>/dev/null");
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // 1. 启动服务器 - 作为独立后台进程
  std::cout << "\n启动服务器..." << std::endl;
  int server_pid = fork();
  if (server_pid == 0) {
    // 子进程：运行服务器
    std::cout << "服务器进程开始运行" << std::endl;
    execl("./build/server/EMS_server", "./build/server/EMS_server", NULL);
    // 如果execl失败
    perror("服务器启动失败");
    exit(1);
  } else if (server_pid > 0) {
    std::cout << "服务器启动成功，PID: " << server_pid << std::endl;
  } else {
    std::cerr << "创建服务器进程失败" << std::endl;
    return 1;
  }

  // 等待服务器初始化
  std::cout << "等待服务器初始化（5秒）..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // 检查服务器是否仍在运行
  if (kill(server_pid, 0) != 0) {
    std::cerr << "服务器进程已退出，请检查服务器日志" << std::endl;
    return 1;
  }

  // 2. 启动模拟器 - 作为独立后台进程
  std::cout << "\n启动模拟器..." << std::endl;
  int simulator_pid = fork();
  if (simulator_pid == 0) {
    // 子进程：运行模拟器
    std::cout << "模拟器进程开始运行" << std::endl;
    execl("./build/equipment_simulation/equipment_simulation",
          "./build/equipment_simulation/equipment_simulation", NULL);
    // 如果execl失败
    perror("模拟器启动失败");
    exit(1);
  } else if (simulator_pid > 0) {
    std::cout << "模拟器启动成功，PID: " << simulator_pid << std::endl;
  } else {
    std::cerr << "创建模拟器进程失败" << std::endl;
    kill(server_pid, SIGTERM);
    return 1;
  }

  // 等待模拟器连接
  std::cout << "等待模拟器连接（5秒）..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // 检查模拟器是否仍在运行
  if (kill(simulator_pid, 0) != 0) {
    std::cerr << "模拟器进程已退出，请检查模拟器日志" << std::endl;
    kill(server_pid, SIGTERM);
    return 1;
  }

  std::cout << "\n=== 系统启动完成 ===" << std::endl;
  std::cout << "服务器PID: " << server_pid << std::endl;
  std::cout << "模拟器PID: " << simulator_pid << std::endl;
  std::cout << "按 Ctrl+C 停止测试" << std::endl;
  std::cout << "\n=== 开始监控心跳 ===" << std::endl;

  // 3. 主循环：监控心跳
  auto start_time = std::chrono::steady_clock::now();
  int seconds_count = 0;

  while (keep_running) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       current_time - start_time)
                       .count();

    // 每10秒打印一次状态
    if (seconds_count % 10 == 0) {
      std::cout << "\n[" << elapsed << "秒] 系统状态:" << std::endl;

      // 检查进程是否仍在运行
      bool server_alive = (kill(server_pid, 0) == 0);
      bool simulator_alive = (kill(simulator_pid, 0) == 0);

      if (!server_alive) {
        std::cout << "❌ 服务器进程已停止" << std::endl;
        break;
      }
      if (!simulator_alive) {
        std::cout << "❌ 模拟器进程已停止" << std::endl;
        break;
      }

      std::cout << "✅ 服务器和模拟器运行正常" << std::endl;

      // 查看进程信息
      std::cout << "进程信息:" << std::endl;
      char cmd[256];
      snprintf(cmd, sizeof(cmd),
               "ps -o pid,etime,cmd -p %d,%d 2>/dev/null | grep -v PID",
               server_pid, simulator_pid);
      system(cmd);

      // 检查心跳日志
      std::cout << "心跳日志（最后2行）:" << std::endl;
      system("grep -i '心跳\\|heartbeat\\|HEARTBEAT' build/server.log "
             "2>/dev/null | tail -5 || echo '无心跳日志'");
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    seconds_count++;

    // 测试时间：2分钟
    if (elapsed >= 120) {
      std::cout << "\n测试时间到（2分钟），准备停止..." << std::endl;
      break;
    }
  }

  // 4. 停止进程
  std::cout << "\n=== 停止测试 ===" << std::endl;

  // 先停止模拟器
  if (kill(simulator_pid, 0) == 0) {
    std::cout << "停止模拟器..." << std::endl;
    kill(simulator_pid, SIGTERM);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));

  // 再停止服务器
  if (kill(server_pid, 0) == 0) {
    std::cout << "停止服务器..." << std::endl;
    kill(server_pid, SIGTERM);
  }

  // 等待进程结束
  std::cout << "等待进程退出..." << std::endl;
  int status;
  waitpid(simulator_pid, &status, 0);
  waitpid(server_pid, &status, 0);

  std::cout << "\n=== 测试完成 ===" << std::endl;
  std::cout << "所有进程已停止" << std::endl;

  return 0;
}