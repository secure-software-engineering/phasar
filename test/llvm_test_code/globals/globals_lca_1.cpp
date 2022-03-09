#include <cstdio>

int createInitialValue() { return 42; }

int foo = createInitialValue();

int main() {
  int x = foo + 1;
  printf("x: %d\n", x);
}
