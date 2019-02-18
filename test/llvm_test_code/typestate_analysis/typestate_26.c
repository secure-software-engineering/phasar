#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  int z;

  if (feof(f) != 0)
    z = 30;
  else
    z = 20;

  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}