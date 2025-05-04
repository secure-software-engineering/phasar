#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // if (argc < 2) return 1;
  //  read data
  int infd = open(argv[1], O_RDONLY);
  long size = lseek(infd, 0, SEEK_END);
  lseek(infd, 0, 0);
  char *buffer = malloc(size + 1);
  read(infd, buffer, size);
  buffer[size] = '\0';
  close(infd);
  // print data
  printf("%s", buffer);
  // write data
  int outfd = open("taint_cpy.txt", O_CREAT);
  write(outfd, buffer, size);
  close(outfd);
  free(buffer);
  return 0;
}
