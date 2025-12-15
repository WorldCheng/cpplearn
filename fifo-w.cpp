#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
  const char *fifo_name = "/tmp/myfifo";

  // 只需要创建一次，存在则忽略错误
  mkfifo(fifo_name, 0666);

  int fd = open(fifo_name, O_WRONLY);
  char msg[100];
  ssize_t read_num;
  while ((read_num = read(STDIN_FILENO, msg, 100)) > 0) {
    write(fd, msg, read_num);
  }
  close(fd);
  return 0;
}
