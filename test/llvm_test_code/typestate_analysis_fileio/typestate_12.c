#include <stdio.h>
#include <stdlib.h>

FILE *bar() {
  FILE *p;
  p = fopen("test", "w+");
  return p;
}

FILE *foo() { return bar(); }

int main() {
  FILE *f;
  f = foo();
  fclose(f);

  return 0;
}
