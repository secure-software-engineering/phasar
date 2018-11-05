#include <stdio.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;
  fclose(f);
  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}