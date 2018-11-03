#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  f = fopen(argv[1], "r");

  foo(f);

  rewind(f);

  return 0;
}