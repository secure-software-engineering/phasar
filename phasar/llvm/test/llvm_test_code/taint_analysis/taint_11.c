#include <stdio.h>

int quk(int i) { return i; }

int baz(int i) { return quk(i); }

int bar(int i) { return baz(i); }

int foo(int i) { return bar(i); }

int main(int argc, char **argv) {
  int y = foo(argc);
  int a = 42;
  int z = foo(a);
  printf("%d\n", y);
  printf("%d\n", z);
}
