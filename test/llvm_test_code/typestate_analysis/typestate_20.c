#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main() {
  FILE *f;

  fprintf(f, "Do something.\n");

  f = fopen("file.txt", "a");

  foo(f);

  return 0;
}