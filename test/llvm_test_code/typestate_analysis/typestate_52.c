#include <stdio.h>
#include <stdlib.h>

int foo(int a) {
  a = a + 2;
  return a;
}

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  z = 42;

  foo(z);

  fclose(f);

  return 0;
}