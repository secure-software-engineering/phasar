#include <cstdio>

int createInitialValue() { return 42; }

int foo = createInitialValue();

int bar;

__attribute__((constructor)) void myGlobalCtor() { bar = 45; }

int main() {
  int x = foo + 1;
  int y = bar - 1;
  printf("x: %d, y: %d\n", x, y);
}
