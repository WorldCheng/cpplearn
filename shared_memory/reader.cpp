#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
  const char *name = "/myshm";
  int fd = shm_open(name, O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(1);
  }

  size_t size = 1024;
  auto addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  printf("reader: %s\n", (char *)addr);

  munmap(addr, size);
  close(fd);
  shm_unlink(name); // 删除共享内存对象
  return 0;
}
