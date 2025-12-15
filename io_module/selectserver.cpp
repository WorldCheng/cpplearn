#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  // 1) 创建监听 socket
  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

  if (::bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (::listen(listen_fd, 128) < 0) {
    perror("listen");
    return 1;
  }

  std::cout << "[select] listening on 0.0.0.0:8080\n";

  // 2) 准备 fd 集合
  fd_set master; // 主集合：记录“我关心哪些 fd”
  FD_ZERO(&master);
  FD_SET(listen_fd, &master);

  int maxfd = listen_fd; // select 需要传 maxfd

  while (true) {
    // select 会修改传入的 fd_set，所以每轮复制一份
    fd_set readfds = master;

    // 3) 阻塞等待：直到某些 fd 可读
    int nready = ::select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
    if (nready < 0) {
      if (errno == EINTR)
        continue;
      perror("select");
      break;
    }

    // 4) 监听 fd 可读 -> 有新连接
    if (FD_ISSET(listen_fd, &readfds)) {
      sockaddr_in cli{};
      socklen_t len = sizeof(cli);
      int conn_fd = ::accept(listen_fd, (sockaddr *)&cli, &len);
      if (conn_fd >= 0) {
        FD_SET(conn_fd, &master);
        if (conn_fd > maxfd)
          maxfd = conn_fd;

        char ip[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        std::cout << "new client " << ip << ":" << ntohs(cli.sin_port)
                  << " fd=" << conn_fd << "\n";
      } else {
        perror("accept");
      }

      // 注意：本轮 ready 数可能还有其他 fd，继续处理
    }

    // 5) 遍历所有 fd（select 的缺点：需要扫一遍）
    for (int fd = 0; fd <= maxfd; ++fd) {
      if (fd == listen_fd)
        continue;
      if (!FD_ISSET(fd, &readfds))
        continue;

      char buf[4096];
      ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
      if (n == 0) {
        // 对端关闭
        std::cout << "client closed fd=" << fd << "\n";
        ::close(fd);
        FD_CLR(fd, &master);
      } else if (n < 0) {
        perror("recv");
        ::close(fd);
        FD_CLR(fd, &master);
      } else {
        // 这里为了教学简单：直接 send 回去（大消息要处理“部分发送”）
        (void)::send(fd, buf, n, 0);
      }
    }
  }

  ::close(listen_fd);
  return 0;
}
