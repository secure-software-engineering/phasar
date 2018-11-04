#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  int z;

  f = fopen(argv[1], "r");

  while ((z = fgetc(f))) {
    if (z == EOF)
      ungetc(z, f);
    else
      fputc(z, stdout);
  }

  foo(f);

  return 0;
}