#include <stdio.h>
#include <stdlib.h>

FILE *foo() {
  FILE *p;
  p = fopen("bar.txt", "w+");
  return p;
}

int main() {
  FILE *f;
  f = foo();
  fprintf(f, "%s %d", "Life is ", 42);
  fclose(f);
  return 0;
}
