// receiver.cpp
#include <cerrno>
#include <cstring>
#include <fcntl.h> // O_RDONLY
#include <iostream>
#include <mqueue.h>
#include <sys/stat.h>

int main() {
  const char *name = "/my_cpp_mq";

  // 打开队列（要求 sender 先创建）
  mqd_t mqd = mq_open(name, O_RDONLY);
  if (mqd == (mqd_t)-1) {
    std::cerr << "mq_open failed: " << std::strerror(errno) << "\n";
    return 1;
  }

  // 查属性，拿到 mq_msgsize
  struct mq_attr attr;
  if (mq_getattr(mqd, &attr) == -1) {
    std::cerr << "mq_getattr failed: " << std::strerror(errno) << "\n";
    mq_close(mqd);
    return 1;
  }

  std::string buf;
  buf.resize(attr.mq_msgsize);

  std::cout << "Receiver started. Waiting messages...\n";

  while (true) {
    unsigned int prio = 0;
    ssize_t n = mq_receive(mqd, buf.data(), buf.size(), &prio);
    if (n == -1) {
      std::cerr << "mq_receive failed: " << std::strerror(errno) << "\n";
      break;
    }

    // mq_receive 返回的 n 包括 '\0'，但保险起见我们手动补一下
    if (n <= (ssize_t)buf.size())
      buf[n - 1] = '\0';

    std::cout << "got(prio=" << prio << "): " << buf.c_str() << "\n";

    if (std::string(buf.c_str()) == "quit") {
      break;
    }
  }

  mq_close(mqd);
  mq_unlink(name); // 这里负责删除队列
  return 0;
}
