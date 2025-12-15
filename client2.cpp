#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
int main() {
  auto fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd) {
    perror("client");
    return 1;
  }
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
  if ((::connect(fd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
    perror("client");
    return 1;
  }
  std::string msg;
  while (std::getline(std::cin, msg)) {
    ::send(fd, msg.data(), msg.size(), 0);
  }
  ::close(fd);
  return 0;
}