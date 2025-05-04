#include <cstdio>

int createInitialValue() { return 42; }

int foo = createInitialValue();

int bar;

__attribute__((constructor)) void myGlobalCtor() { bar = 45; }

int main(int Argc, char **Argv) {
  Argc = foo + 1;
  int y = bar - 1;
  return 0;
}
