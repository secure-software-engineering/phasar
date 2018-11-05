#include <stdio.h>
#include <stdlib.h>

FILE *foo2(FILE *f, FILE *f2) {
  f2 = f;
  return f2;
}

void foo(FILE *f, char **argv) {
  FILE *f2;
  freopen(argv[1], "r", f);

  f2 = foo2(f, f2);

  fclose(f2);
}

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  z = 42;

  foo(f, argv);

  fclose(f);

  return 0;
}