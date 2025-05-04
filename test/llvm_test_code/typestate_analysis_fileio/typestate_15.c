#include <stdio.h>

int main() {
  FILE *f;
  f = fopen("foo.txt", "r");
  fclose(f);
  f = fopen("bar.txt", "w");
  fclose(f);

  return 0;
}
