#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f, char **argv, int *z) {
  freopen(argv[1], "r", f);
  *z = 30;
  fscanf(f, "%d", z);
}

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  z = 42;

  foo(f, argv, &z);

  fclose(f);

  return 0;
}