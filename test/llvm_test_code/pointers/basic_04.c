#include <stdlib.h>

int main() {
  char *i = (char *)(malloc(3 * sizeof(char)));
  char **p = &i;
  *p[0] = 'A';
  *p[1] = 'B';
  *p[2] = 'C';
  free(i);
}
