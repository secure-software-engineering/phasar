#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  fseek(f, 0L, SEEK_SET); // Jumps to the Beginning of the file.

  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}