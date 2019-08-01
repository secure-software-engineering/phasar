#include <cstdio>
#include <cstring>
int foo(int i) { return i; }
int main(int argc, char *argv[]) {
  int (*bar)(int);
  if (argc > 2) {
    bar = &foo;
  } else {
    bar = &putchar;
  }

  return bar(argc);
}