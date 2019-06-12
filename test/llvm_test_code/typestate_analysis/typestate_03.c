#include <stdio.h>

void foo(FILE *f) { fclose(f); }

int main() {
  FILE *f;
  f = fopen("file.txt", "r");

  foo(f);

  return 0;
}