#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <print>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
public:
  // 构造函数：指定线程数量
  explicit ThreadPool(
      std::size_t threadCount = std::thread::hardware_concurrency())
      : stop_(false) {
    if (threadCount == 0) {
      threadCount = 1;
    }

    // 创建工作线程
    for (std::size_t i = 0; i < threadCount; ++i) {
      workers_.emplace_back([this] {
        for (;;) {
          std::function<void()> task;

          { // 取任务的临界区
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            // 池子要结束并且没有任务了，退出线程
            if (stop_ && tasks_.empty()) {
              return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
          }

          // 执行任务（可以在这里做统一异常捕获）
          try {
            task();
          } catch (...) {
            // 你可以在这里写日志 / 统计
          }
        }
      });
    }
  }

  // 禁止拷贝
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // 允许移动（可选）
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  // 析构：通知结束 + 等待所有线程退出
  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cond_.notify_all();
    for (auto &t : workers_) {
      if (t.joinable())
        t.join();
    }
  }

  // 提交任务，返回 std::future<返回值类型>
  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    // 把可调用对象和参数 bind 到一起，放到 packaged_task 里
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stop_) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      tasks_.emplace([task]() { (*task)(); });
    }
    cond_.notify_one();
    return res;
  }

private:
  std::vector<std::thread> workers_;        // 工作线程
  std::queue<std::function<void()>> tasks_; // 任务队列

  std::mutex mutex_;
  std::condition_variable cond_;
  bool stop_;
};

int main() {
  ThreadPool pool(4); // 4 个工作线程

  std::vector<std::future<int>> results;

  for (int i = 0; i < 8; ++i) {
    // 提交任务：计算 i * i
    results.emplace_back(pool.enqueue([i] {
      std::println("task {} running in thread {}", i,
                   std::hash<std::thread::id>{}(std::this_thread::get_id()));
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      return i * i;
    }));
  }

  // 等待并打印结果
  for (auto &f : results) {
    std::println("result = {}", f.get());
  }

  // main 结束时，pool 析构，会自动优雅停止所有线程
  return 0;
}