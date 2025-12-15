// mq_timeout_demo_fixed.c
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define QUEUE_NAME "/mq_demo_timeout"

int main(void) {
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 2;
  attr.mq_msgsize = 64;
  attr.mq_curmsgs = 0;

  mqd_t mqd = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0666, &attr);
  if (mqd == (mqd_t)-1) {
    perror("mq_open");
    return 1;
  }

  printf("队列创建成功，最大消息数 = %ld\n", attr.mq_maxmsg);

  const char *msg1 = "msg-1";
  const char *msg2 = "msg-2";
  const char *msg3 = "msg-3 (will timeout)";

  if (mq_send(mqd, msg1, strlen(msg1) + 1, 0) == -1) {
    perror("mq_send 1");
    mq_close(mqd);
    mq_unlink(QUEUE_NAME);
    return 1;
  }
  printf("已发送: %s\n", msg1);

  if (mq_send(mqd, msg2, strlen(msg2) + 1, 0) == -1) {
    perror("mq_send 2");
    mq_close(mqd);
    mq_unlink(QUEUE_NAME);
    return 1;
  }
  printf("已发送: %s\n", msg2);

  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    perror("clock_gettime");
    mq_close(mqd);
    mq_unlink(QUEUE_NAME);
    return 1;
  }
  ts.tv_sec += 5;

  printf("\n队列已满，调用 mq_timedsend 发送第三条消息，最多等待 5 秒...\n");

  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);

  int ret = mq_timedsend(mqd, msg3, strlen(msg3) + 1, 0, &ts);

  clock_gettime(CLOCK_REALTIME, &end);

  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

  if (ret == -1) {
    if (errno == ETIMEDOUT) {
      printf("mq_timedsend 超时返回，errno = ETIMEDOUT\n");
      printf("实际等待时间约为：%.3f 秒\n", elapsed);
    } else {
      perror("mq_timedsend 失败");
    }
  } else {
    printf("mq_timedsend 成功发送了第三条消息：%s\n", msg3);
  }

  mq_close(mqd);
  mq_unlink(QUEUE_NAME);
  return 0;
}
