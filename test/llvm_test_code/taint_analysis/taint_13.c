#include <stdio.h>

int foo(int i) { return i; }

int main(int argc, char **argv) {
  int y = foo(argc);
  int a = 42;
  while (argc-- > 0) {
    a += 42;
  }
  int z = foo(a);
  printf("%d\n", y);
  printf("%d\n", z);
}
