#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  f = fopen(argv[1], "r");

  fseek(f, 0L, SEEK_END);

  foo(f);

  printf("The size of the file is %ld bytes.\n", ftell(f));

  return 0;
}