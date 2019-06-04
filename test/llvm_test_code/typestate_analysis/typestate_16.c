#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = getc(f)) != EOF)
    putc(i, f);

  fclose(f);
}

int main() {
  FILE *f;
  f = fopen("file.txt", "a");

  foo(f);

  return 0;
}