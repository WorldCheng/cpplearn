#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

int main() {
  std::queue<std::packaged_task<int()>> tasks;
  std::mutex m;
  std::condition_variable cv;
  bool done = false;

  // worker 线程
  std::thread worker([&] {
    while (true) {
      std::packaged_task<int()> task;
      {
        std::unique_lock lock(m);
        cv.wait(lock, [&] { return done || !tasks.empty(); });
        if (done && tasks.empty())
          break;
        task = std::move(tasks.front());
        tasks.pop();
      }
      // 真正执行任务，这里会计算并填充 future
      task();
    }
  });

  // 提交任务 1
  {
    std::packaged_task<int()> task([] { return 10; });
    auto f = task.get_future(); // future1

    {
      std::lock_guard lock(m);
      tasks.push(std::move(task));
    }
    cv.notify_one();

    std::cout << "task1 result = " << f.get() << '\n';
  }

  // 提交任务 2
  {
    std::packaged_task<int()> task([] { return 20; });
    auto f = task.get_future(); // future2

    {
      std::lock_guard lock(m);
      tasks.push(std::move(task));
    }
    cv.notify_one();

    std::cout << "task2 result = " << f.get() << '\n';
  }

  // 通知结束
  {
    std::lock_guard lock(m);
    done = true;
  }
  cv.notify_one();

  worker.join();
}
