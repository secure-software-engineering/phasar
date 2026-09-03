#include <stdio.h>
int main() {
  FILE *f = fopen("data.txt", "r");
  fclose(f);
  int c = fgetc(f); /* BUG: use of FILE* after fclose */
  printf("%d\n", c);
  return 0;
}
