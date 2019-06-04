#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main() {
  FILE *f;

  int i;

  f = fopen("file.txt", "a");

  foo(f);

  while ((i = getc(f)) != EOF)
    putc(i, f);

  return 0;
}