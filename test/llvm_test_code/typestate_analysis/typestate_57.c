#include <stdio.h>
#include <stdlib.h>

void foo(FILE *p) {
  p = fopen("bar.txt", "w+");
}

int main() {
  FILE *f;
  foo(f);
  // fprintf(f, "%s %d", "Life is ", 42);
  fclose(f);
  return 0;
}
