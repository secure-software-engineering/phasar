#include <stdio.h>

// Fails since f and d are assumed to be may-alias
int main() {
  FILE *f;
  FILE *d;
  f = fopen("foo.txt", "r");
  d = fopen("bar.txt", "w");

  fclose(f);

  return 0;
}