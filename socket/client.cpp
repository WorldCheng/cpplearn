#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    return 1;
  }

  std::string line;
  char buf[4096];
  while (std::getline(std::cin, line)) {
    line.push_back('\n');
    ::send(fd, line.data(), line.size(), 0);

    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0)
      break;
    std::cout.write(buf, n);
  }

  ::close(fd);
}
