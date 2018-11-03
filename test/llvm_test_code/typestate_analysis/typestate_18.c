#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  int i;

  f = fopen(argv[1], "a");

  foo(f);

  while ((i = getc(f)) != EOF)
    putc(i, f);

  return 0;
}