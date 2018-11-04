#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;
  int x;

  x = 7;

  f = fopen(argv[1], "a");

  foo(f);

  fscanf(f, "%d", &x);

  return 0;
}