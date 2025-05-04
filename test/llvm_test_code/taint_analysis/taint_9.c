#include <stdio.h>

int foo(int i) { return i; }

int main(int argc, char **argv) {
  int y = foo(argc);
  printf("%d\n", y);
}
