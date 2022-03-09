#include <stdio.h>

int main() {
  FILE *f;
  f = fopen("foo.txt", "r");
  f = fopen("bar.txt", "w");
  fclose(f);

  return 0;
}
