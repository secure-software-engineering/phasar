#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = fgetc(f)) != EOF)
    putchar(i);

  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;
  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}