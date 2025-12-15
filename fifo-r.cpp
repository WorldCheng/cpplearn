#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
  const char *fifo_name = "/tmp/myfifo";
  char buf[100];

  mkfifo(fifo_name, 0666); // 同上，可容忍已存在

  int fd = open(fifo_name, O_RDONLY);
  ssize_t read_num;
  while ((read_num = read(fd, buf, 100)) > 0) {
    write(STDOUT_FILENO, buf, read_num);
  }
  close(fd);
  return 0;
}
