#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f, int *x) {
  fscanf(f, "%d", x);
  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;
  int x;

  f = fopen(argv[1], "a");

  x = 42;

  foo(f, &x);

  return 0;
}