#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f, fpos_t position) {
  fgetpos(f, &position);
  fsetpos(f, &position);
  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;

  fpos_t position;

  f = fopen(argv[1], "r");

  foo(f, position);

  return 0;
}