#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  // if (argc < 2) return 1;
  //  read data
  FILE *inf = fopen(argv[1], "r");
  fseek(inf, 0, SEEK_END);
  long size = ftell(inf);
  rewind(inf);
  char *buffer = malloc(size + 1);
  fread(buffer, size, 1, inf);
  buffer[size] = '\0';
  fclose(inf);
  // print data
  printf("%s", buffer);
  // write data
  FILE *outf = fopen("taint_cpy.txt", "a+");
  fwrite(buffer, size, 1, outf);
  fclose(outf);
  free(buffer);
  return 0;
}
