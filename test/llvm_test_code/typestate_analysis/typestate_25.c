#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = getc(f))) {
    if (feof(f) != 0)
      break;
    else
      putc(i, f);
  }

  fclose(f);
}

int main(int argc, char **argv) {
  FILE *f;
  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}