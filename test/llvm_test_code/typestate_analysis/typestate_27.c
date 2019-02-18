#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  foo(f);

  if (feof(f) != 0)
    z = 30;
  else
    z = 20;

  return 0;
}