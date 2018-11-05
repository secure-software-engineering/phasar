#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  int i;
  FILE *f;

  while ((i = fgetc(f)) != EOF)
    ;

  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}