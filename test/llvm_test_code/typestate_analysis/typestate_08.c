#include <stdio.h>
#include <stdlib.h>

FILE *foo() {
  FILE *p;
  p = fopen("bar.txt", "w+");
  return p;
}

int main() {
  FILE *f;
  foo();
  return 0;
}
