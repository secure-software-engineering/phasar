#include <stdio.h>

extern int cond;

int main() {
  FILE *f;
  f = fopen("file.txt", "r");
  if (cond) {
    fclose(f);
  }
  return 0;
}
