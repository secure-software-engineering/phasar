#include <stdio.h>

int main() {
  FILE *f;
  f = fopen("file.txt", "r");
  fclose(f);
  return 0;
}
