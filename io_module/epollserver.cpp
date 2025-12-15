#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

static bool set_nonblocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return false;
  return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

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

  // epoll + 非阻塞是典型组合
  if (!set_nonblocking(listen_fd)) {
    perror("fcntl nonblock listen");
    return 1;
  }

  int epfd = ::epoll_create1(EPOLL_CLOEXEC);
  if (epfd < 0) {
    perror("epoll_create1");
    return 1;
  }

  // 把 listen_fd 加入 epoll
  epoll_event ev{};
  ev.events = EPOLLIN; // 关心可读：表示有新连接
  ev.data.fd = listen_fd;
  if (::epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
    perror("epoll_ctl ADD listen");
    return 1;
  }

  std::cout << "[epoll] listening on 0.0.0.0:8080\n";

  std::vector<epoll_event> events(64);

  while (true) {
    int n = ::epoll_wait(epfd, events.data(), (int)events.size(), -1);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      uint32_t re = events[i].events;

      if (fd == listen_fd) {
        // listen_fd 可读：accept 可能不止一个（非阻塞下要循环 accept）
        while (true) {
          sockaddr_in cli{};
          socklen_t len = sizeof(cli);
          int conn_fd = ::accept(listen_fd, (sockaddr *)&cli, &len);
          if (conn_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break; // 没有更多连接
            perror("accept");
            break;
          }

          set_nonblocking(conn_fd);

          // 把新连接加入 epoll，关心读 + 对端关闭
          epoll_event cev{};
          cev.events = EPOLLIN | EPOLLRDHUP;
          cev.data.fd = conn_fd;
          if (::epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &cev) < 0) {
            perror("epoll_ctl ADD conn");
            ::close(conn_fd);
            continue;
          }

          char ip[INET_ADDRSTRLEN];
          ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
          std::cout << "new client " << ip << ":" << ntohs(cli.sin_port)
                    << " fd=" << conn_fd << "\n";
        }
        continue;
      }

      // 处理连接异常/对端关闭
      if (re & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
        ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        ::close(fd);
        continue;
      }

      if (re & EPOLLIN) {
        // 可读：非阻塞下建议循环 recv，直到 EAGAIN
        while (true) {
          char buf[4096];
          ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
          if (r > 0) {
            // 教学简化：直接回写。若 send 返回
            // EAGAIN，生产环境要做“发送缓冲+关注 EPOLLOUT”
            ssize_t s = ::send(fd, buf, (size_t)r, 0);
            (void)s;
            continue;
          }
          if (r == 0) {
            // 对端正常关闭
            ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
            break;
          }
          // r < 0
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 本轮数据读完了
            break;
          }
          perror("recv");
          ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
          ::close(fd);
          break;
        }
      }
    }
  }

  ::close(epfd);
  ::close(listen_fd);
  return 0;
}
