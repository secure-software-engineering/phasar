#include <stdio.h>
#include <stdlib.h>

void foo2(FILE *f, FILE *f2) { f2 = f; }

void foo(FILE *f, char **argv, int *z) {
  FILE *f2;
  freopen(argv[1], "r", f);
  *z = 30;
  fscanf(f, "%d", z);

  foo2(f, f2);

  fclose(f2);
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