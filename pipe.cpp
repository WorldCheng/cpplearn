#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  int fd[2];
  if (pipe(fd) == -1) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  } else if (pid == 0) { // 子进程
    close(fd[1]);        // 关闭写端
    char buf[100] = {0};
    read(fd[0], buf, sizeof(buf));
    printf("child received: %s\n", buf);
    close(fd[0]);
  } else {        // 父进程
    close(fd[0]); // 关闭读端
    const char *msg = "hello from parent";
    write(fd[1], msg, strlen(msg) + 1);
    close(fd[1]);
  }
  waitpid(pid, NULL, 0);
  return 0;
}
