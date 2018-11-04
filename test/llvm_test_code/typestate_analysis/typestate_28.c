#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  if (ferror(f)) {
    z = -1;
  }

  foo(f);

  return 0;
}