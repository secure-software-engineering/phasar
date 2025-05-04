#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main() {
  int i;
  FILE *f;

  f = fopen("foo.txt", "r");

  foo(f);

  while ((i = fgetc(f)) != EOF) {
  }

  return 0;
}
