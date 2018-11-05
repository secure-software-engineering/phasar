#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  f = fopen(argv[1], "a");

  foo(f);

  fprintf(f, "Do something.\n");

  return 0;
}