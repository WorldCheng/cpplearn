/*
  封装“线程基类接口”的一个常见写法
*/

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

class ThreadBase {
public:
  ThreadBase() = default;
  virtual ~ThreadBase() {
    // 确保析构前线程结束，避免 std::terminate
    if (worker_.joinable()) {
      stop();
      worker_.join();
    }
  }

  // 禁止拷贝，允许移动（简单起见，这里只禁拷贝）
  ThreadBase(const ThreadBase &) = delete;
  ThreadBase &operator=(const ThreadBase &) = delete;

  // 启动线程
  void start() {
    if (running_)
      return; // 已经在跑就不重复启动
    stopFlag_ = false;
    running_ = true;
    worker_ = std::thread(&ThreadBase::threadEntry, this); // 成员函数作为入口
  }

  // 请求停止（协作式）
  void stop() { stopFlag_ = true; }

  // 等待线程结束
  void join() {
    if (worker_.joinable()) {
      worker_.join();
    }
    running_ = false;
  }

  bool isRunning() const noexcept { return running_; }

protected:
  // 派生类实现这个函数，里面写真正的逻辑
  virtual void run() = 0;

  // 给派生类检查停止的机会
  bool shouldStop() const noexcept { return stopFlag_; }

private:
  // 真正给 std::thread 用的入口函数
  void threadEntry() {
    try {
      run(); // 调用派生类实现
    } catch (...) {
      // 这里可以做统一的异常处理 / 日志
    }
    running_ = false;
  }

  std::thread worker_;
  std::atomic_bool stopFlag_{false};
  std::atomic_bool running_{false};
};

class MyWorker : public ThreadBase {
public:
  MyWorker(int id) : id_(id) {}

protected:
  void run() override {
    // 一个简单示例：每 500ms 打印一次，直到 shouldStop()
    while (!shouldStop()) {
      std::cout << "Worker " << id_ << " is running in thread "
                << std::this_thread::get_id() << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Worker " << id_ << " stopped.\n";
  }

private:
  int id_;
};

int main() {
  MyWorker w(1);
  w.start(); // 启动线程

  std::this_thread::sleep_for(std::chrono::seconds(2));

  w.stop(); // 请求停止
  w.join(); // 等待线程退出

  return 0;
}
