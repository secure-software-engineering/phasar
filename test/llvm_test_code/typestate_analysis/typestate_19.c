#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  fprintf(f, "Do something.\n");
  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;

  f = fopen(argv[1], "a");

  foo(f);

  return 0;
}