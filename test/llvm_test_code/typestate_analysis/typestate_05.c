#include <stdio.h>

int main() {
  FILE *f;
  FILE *d;
  f = fopen("file.txt", "r");
  d = fopen("file.txt", "r");

  fclose(f);

  return 0;
}