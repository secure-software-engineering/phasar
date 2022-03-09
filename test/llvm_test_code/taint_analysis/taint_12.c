#include <stdio.h>

int foo(int i, int j) {
  if (j < 1) {
    return i;
  }
  return foo(i, --j);
}

int main(int argc, char **argv) {
  int y = foo(argc, 10);
  int a = 42;
  int z = foo(a, 10);
  printf("%d\n", y);
  printf("%d\n", z);
}
