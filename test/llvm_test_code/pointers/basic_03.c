#include <stdlib.h>

int main() {
  char *i = (char *)(malloc(3 * sizeof(char)));
  i[0] = 'A';
  i[1] = 'B';
  i[2] = 'C';
  free(i);
}
