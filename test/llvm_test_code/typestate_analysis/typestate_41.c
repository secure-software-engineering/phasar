#include <stdio.h>
#include <stdlib.h>

void foo(FILE *f) { fclose(f); }

int main(int argc, char **argv) {
  FILE *f;

  printf("You are at position %ld.\n", ftell(f));

  f = fopen(argv[1], "r");

  foo(f);

  return 0;
}