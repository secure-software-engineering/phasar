#include <cstdio>

int createInitialFoo() { return 42; }

int getFoo() {
  static int foo = createInitialFoo();
  return foo;
}

int main() {
  int x = getFoo() + 1;
  printf("x: %d\n", x);
}