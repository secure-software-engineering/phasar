#include <stdio.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;
  f = fopen(argv[1], "r");

  foo(f);

  freopen(argv[1], "r", f);

  foo(f);

  return 0;
}