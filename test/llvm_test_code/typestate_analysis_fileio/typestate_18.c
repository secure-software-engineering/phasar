#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = fgetc(f)) != EOF)
    fputc(i, f);

  fclose(f);
}

int main() {
  FILE *f;
  f = fopen("file.txt", "a");

  foo(f);

  return 0;
}
