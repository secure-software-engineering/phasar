#include <cstdio>
#include <cstring>
int foo(int i) { return i; }
int oof(int i) { return putchar(i); }
int main(int argc, char *argv[]) {
  int (*bar)(int);
  if (argc > 2) {
    bar = &foo;
  } else {
    bar = &oof;
  }

  return bar(argc);
}
