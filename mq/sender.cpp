// sender.cpp
#include <cerrno>  // errno
#include <cstring> // strerror
#include <fcntl.h> // O_CREAT, O_WRONLY
#include <iostream>
#include <mqueue.h>
#include <string>
#include <sys/stat.h> // mode_t

int main() {
  const char *name = "/my_cpp_mq";

  // 队列属性（可以也用 nullptr 用默认属性）
  struct mq_attr attr;
  attr.mq_flags = 0;     // 阻塞
  attr.mq_maxmsg = 10;   // 最多 10 条消息
  attr.mq_msgsize = 256; // 每条最多 256 字节
  attr.mq_curmsgs = 0;

  mqd_t mqd = mq_open(name, O_CREAT | O_WRONLY, 0666, &attr);
  if (mqd == (mqd_t)-1) {
    std::cerr << "mq_open failed: " << std::strerror(errno) << "\n";
    return 1;
  }

  std::cout << "Sender started. Input messages, 'quit' to exit.\n";

  std::string line;
  while (true) {
    std::cout << ">> ";
    if (!std::getline(std::cin, line))
      break;

    if (mq_send(mqd, line.c_str(), line.size() + 1, 0) == -1) {
      std::cerr << "mq_send failed: " << std::strerror(errno) << "\n";
      break;
    }
    if (line == "quit")
      break;
  }

  mq_close(mqd);
  // 不 unlink，让 receiver 去删
  return 0;
}
