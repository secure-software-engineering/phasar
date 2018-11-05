#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = fgetc(f)) != EOF)
    fputc(i, f);

  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;
  f = fopen(argv[1], "a");

  foo(f);

  return 0;
}