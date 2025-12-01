#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

// 简单的测试程序，运行后监控心跳情况
int main() {
  std::cout << "=== 心跳机制验证测试 ===" << std::endl;
  std::cout << "测试步骤：" << std::endl;
  std::cout << "1. 启动服务器" << std::endl;
  std::cout << "2. 启动模拟器" << std::endl;
  std::cout << "3. 等待2分钟,观察心跳日志" << std::endl;
  std::cout << "4. 检查心跳是否正常发送和响应" << std::endl;

  // 启动服务器（假设在build目录）
  std::cout << "\n正在启动服务器..." << std::endl;
  system("./build/server/EMS_server &");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // 启动模拟器
  std::cout << "正在启动模拟器..." << std::endl;
  system("./build/equipment_simulation/equipment_simulation &");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "\n=== 开始心跳监控 ===" << std::endl;
  std::cout << "监控时间: 2分钟" << std::endl;
  std::cout << "按 Ctrl+C 提前结束" << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  int check_interval = 10; // 每10秒检查一次

  while (true) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       current_time - start_time)
                       .count();

    if (elapsed >= 120) { // 2分钟
      break;
    }

    std::cout << "\n[" << elapsed << "秒] 当前状态:" << std::endl;

    // 检查进程是否还在运行
    int server_status = system("pgrep -f EMS_server > /dev/null");
    int simulator_status = system("pgrep -f equipment_simulation > /dev/null");

    if (server_status != 0) {
      std::cout << "❌ 服务器进程已停止" << std::endl;
      break;
    }

    if (simulator_status != 0) {
      std::cout << "❌ 模拟器进程已停止" << std::endl;
      break;
    }

    std::cout << "✅ 服务器和模拟器进程运行正常" << std::endl;

    // 检查心跳相关日志（需要查看日志文件或输出）
    std::cout << "最近日志（最后3行）:" << std::endl;
    system(
        "ps aux | grep -E '(EMS_server|equipment_simulation)' | grep -v grep");

    std::this_thread::sleep_for(std::chrono::seconds(check_interval));
  }

  // 清理进程
  std::cout << "\n=== 测试结束，清理进程 ===" << std::endl;
  system("pkill -f EMS_server");
  system("pkill -f equipment_simulation");

  std::cout << "心跳测试完成" << std::endl;

  // 总结检查点
  std::cout << "\n=== 检查点总结 ===" << std::endl;
  std::cout << "1. 进程是否稳定运行？" << std::endl;
  std::cout << "2. 是否有心跳发送/接收日志？" << std::endl;
  std::cout << "3. 连接是否保持活跃？" << std::endl;
  std::cout << "4. 是否有异常断开？" << std::endl;

  return 0;
}