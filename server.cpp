#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  auto server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("server_fd");
    return 1;
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt = 1;
  ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if ((::bind(server_fd, (const struct sockaddr *)&addr, sizeof(addr))) < 0) {
    perror("bind");
    return 1;
  }

  if ((::listen(server_fd, 12)) < 0) {
    perror("listen");
    return 1;
  }

  std::println("server on 0.0.0.0:8080");

  sockaddr_in cli{};
  socklen_t len = sizeof(cli);
  auto conn_fd = ::accept(server_fd, (struct sockaddr *)&cli, &len);
  if (conn_fd < 0) {
    perror("accept");
    return 1;
  }
  std::println("client: {}:{}", ::inet_ntoa(cli.sin_addr),
               ::ntohs(cli.sin_port));
  while (true) {
    char buf[1024];
    auto n = ::read(conn_fd, buf, sizeof(buf));
    if (n <= 0) {
      std::println("client disconnected");
      break;
    }
    std::println("{}", std::string_view{buf, static_cast<size_t>(n)});
  }
  ::close(conn_fd);
  ::close(server_fd);
  std::println("server exit");
  return 0;
}