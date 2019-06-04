#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) {
  fprintf(f, "Do something.\n");
  fclose(f);
}

int main() {
  FILE *f;

  f = fopen("file.txt", "a");

  foo(f);

  return 0;
}