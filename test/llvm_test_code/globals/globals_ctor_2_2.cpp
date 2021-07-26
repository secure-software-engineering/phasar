#include <cstdio>

#include "globals_ctor_2_1.h"

int createBar() { return 234765; }

int bar = createBar();

int main() {
  printf("Foo: %d\n", foo);
  printf("Bar: %d\n", bar);
}