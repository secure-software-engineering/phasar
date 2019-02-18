#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  fseek(f, 0L, SEEK_END);
  printf("The size of the file is %ld bytes.\n", ftell(f));
  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;

  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}