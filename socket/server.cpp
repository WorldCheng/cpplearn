#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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

  std::cout << "listening on 0.0.0.0:8080\n";

  sockaddr_in cli{};
  socklen_t len = sizeof(cli);
  int conn_fd = ::accept(listen_fd, (sockaddr *)&cli, &len);
  if (conn_fd < 0) {
    perror("accept");
    return 1;
  }

  char ip[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
  std::cout << "client: " << ip << ":" << ntohs(cli.sin_port) << "\n";

  char buf[4096];
  while (true) {
    ssize_t n = ::recv(conn_fd, buf, sizeof(buf), 0);
    if (n == 0) {
      std::cout << "client closed\n";
      break;
    }
    if (n < 0) {
      perror("recv");
      break;
    }

    // 原样写回（注意：实际生产要处理“部分发送”，这里先简化）
    ssize_t sent = ::send(conn_fd, buf, n, 0);
    if (sent < 0) {
      perror("send");
      break;
    }
  }

  ::close(conn_fd);
  ::close(listen_fd);
}
