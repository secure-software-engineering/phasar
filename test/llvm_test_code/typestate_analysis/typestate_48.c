#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  fpos_t position;

  f = fopen(argv[1], "r");

  foo(f);

  fgetpos(f, &position);
  fsetpos(f, &position);

  return 0;
}