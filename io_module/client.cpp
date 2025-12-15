#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// 可靠发送：把 buf 全部发出去（处理部分发送/EINTR）
static bool send_all(int fd, const void *buf, size_t len) {
  const char *p = static_cast<const char *>(buf);
  while (len > 0) {
    ssize_t n = ::send(fd, p, len, 0);
    if (n > 0) {
      p += n;
      len -= (size_t)n;
      continue;
    }
    if (n < 0 && errno == EINTR)
      continue;
    return false;
  }
  return true;
}

// 打印“本端/对端”地址（便于排错）
static void print_sock_pair(int fd) {
  sockaddr_storage local{}, peer{};
  socklen_t llen = sizeof(local), plen = sizeof(peer);

  if (::getsockname(fd, (sockaddr *)&local, &llen) == 0 &&
      ::getpeername(fd, (sockaddr *)&peer, &plen) == 0) {

    auto to_str = [](const sockaddr_storage &ss) -> std::string {
      char host[NI_MAXHOST], serv[NI_MAXSERV];
      int rc =
          ::getnameinfo((const sockaddr *)&ss, sizeof(ss), host, sizeof(host),
                        serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
      if (rc != 0)
        return "unknown";
      return std::string(host) + ":" + serv;
    };

    std::cout << "local = " << to_str(local) << "\n";
    std::cout << "peer  = " << to_str(peer) << "\n";
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <host_or_ip> <port>\n"
              << "Example: " << argv[0] << " 127.0.0.1 8080\n"
              << "Example: " << argv[0] << " example.com 80\n";
    return 1;
  }

  const char *host = argv[1];
  const char *port = argv[2];

  // 1) DNS/地址解析：支持域名/IPv4/IPv6
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;     // IPv4/IPv6 都可
  hints.ai_socktype = SOCK_STREAM; // TCP

  addrinfo *res = nullptr;
  int rc = ::getaddrinfo(host, port, &hints, &res);
  if (rc != 0) {
    std::cerr << "getaddrinfo: " << ::gai_strerror(rc) << "\n";
    return 1;
  }

  int fd = -1;

  // 2) 遍历候选地址，尝试 connect（有些地址可能不可达）
  for (addrinfo *p = res; p; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd < 0)
      continue;

    if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
      break; // 成功
    }

    ::close(fd);
    fd = -1;
  }

  ::freeaddrinfo(res);

  if (fd < 0) {
    perror("connect");
    return 1;
  }

  std::cout << "connected.\n";
  print_sock_pair(fd);

  // 3) 交互：stdin 读一行 -> 发给服务器 -> 读回显并打印
  std::string line;
  std::vector<char> buf(4096);

  while (std::getline(std::cin, line)) {
    line.push_back('\n');

    if (!send_all(fd, line.data(), line.size())) {
      std::cerr << "send failed\n";
      break;
    }

    // Echo server 通常会回一段数据，这里简单 recv 一次演示
    ssize_t n = ::recv(fd, buf.data(), buf.size(), 0);
    if (n == 0) {
      std::cout << "server closed\n";
      break;
    }
    if (n < 0) {
      perror("recv");
      break;
    }
    std::cout.write(buf.data(), n);
    std::cout.flush();
  }

  ::close(fd);
  return 0;
}
