#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

int main(int argc, char *argv[]) {
  int fd = open("l.txt", O_RDWR | O_CREAT, 0644);
  flock(fd, LOCK_EX);
  while (1) {
    write(fd, "Writing to locked file\n", 22);
    sleep(1);
  }
}
