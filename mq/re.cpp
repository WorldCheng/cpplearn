#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define QUEUE_NAME "/mq_demo_timeout"
int main() {
  auto mqd = mq_open(QUEUE_NAME, O_RDONLY);
  struct mq_attr attr;
  if (mq_getattr(mqd, &attr) == -1) {
    perror("mq_getattr");
    exit(EXIT_FAILURE);
  }
  char *buf = static_cast<char *>(malloc(attr.mq_msgsize));

  while (1) {
    sleep(3);
    unsigned int prio;
    ssize_t n = mq_receive(mqd, buf, attr.mq_msgsize, &prio);
    if (n == -1) {
      perror("mq_receive");
      break;
    }
    printf("Received (prio=%u): %s\n", prio, buf);

    if (strcmp(buf, "quit") == 0) {
      break;
    }
  }
  return 0;
}