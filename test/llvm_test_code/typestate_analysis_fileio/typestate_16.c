#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  int i;

  while ((i = fgetc(f)) != EOF)
    putchar(i);

  fclose(f);
}

int main() {
  FILE *f;
  f = fopen("foo.txt", "r");

  foo(f);

  return 0;
}
