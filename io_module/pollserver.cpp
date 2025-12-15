#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

int main() {
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
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (::bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (::listen(listen_fd, 128) < 0) {
    perror("listen");
    return 1;
  }

  std::cout << "[poll] listening on 0.0.0.0:8080\n";

  // poll 用一个数组（vector）管理所有 fd
  std::vector<pollfd> fds;
  fds.push_back({listen_fd, POLLIN, 0}); // 监听 fd 只关心可读（新连接）

  while (true) {
    int nready = ::poll(fds.data(), (nfds_t)fds.size(), -1);
    if (nready < 0) {
      if (errno == EINTR)
        continue;
      perror("poll");
      break;
    }

    // 1) 先处理监听 fd
    if (fds[0].revents & POLLIN) {
      sockaddr_in cli{};
      socklen_t len = sizeof(cli);
      int conn_fd = ::accept(listen_fd, (sockaddr *)&cli, &len);
      if (conn_fd >= 0) {
        fds.push_back({conn_fd, POLLIN, 0});
        char ip[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        std::cout << "new client " << ip << ":" << ntohs(cli.sin_port)
                  << " fd=" << conn_fd << "\n";
      } else {
        perror("accept");
      }
    }

    // 2) 处理客户端 fd（从 1 开始，因为 0 是 listen_fd）
    for (size_t i = 1; i < fds.size();) {
      auto &p = fds[i];

      // 对端关闭/错误/挂起，也会出现在 revents
      if (p.revents & (POLLHUP | POLLERR | POLLNVAL)) {
        ::close(p.fd);
        // 删除：与末尾交换再 pop_back，O(1)
        p = fds.back();
        fds.pop_back();
        continue;
      }

      if (p.revents & POLLIN) {
        char buf[4096];
        ssize_t n = ::recv(p.fd, buf, sizeof(buf), 0);
        if (n == 0) {
          std::cout << "client closed fd=" << p.fd << "\n";
          ::close(p.fd);
          p = fds.back();
          fds.pop_back();
          continue;
        } else if (n < 0) {
          perror("recv");
          ::close(p.fd);
          p = fds.back();
          fds.pop_back();
          continue;
        } else {
          (void)::send(p.fd, buf, n, 0);
        }
      }

      ++i;
    }
  }

  ::close(listen_fd);
  return 0;
}
