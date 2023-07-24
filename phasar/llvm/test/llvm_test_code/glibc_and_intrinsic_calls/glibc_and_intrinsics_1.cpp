#include <cstdlib>

int foo() { return 13; }

int main() {
  int *i = (int *)malloc(sizeof(int));
  *i = 13;
  int j = foo();
  j = j + *i;
  free(i);
  return 0;
}
