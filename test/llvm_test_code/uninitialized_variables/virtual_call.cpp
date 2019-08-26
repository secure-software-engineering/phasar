#include <cstdio>

int &foo(int &x) { return x; }
int &bar(int &x) {
  x = 42;
  return x;
}
int main(int argc, char *argv[]) {
  int i;
  int &(*baz)(int &);
  if (argc > 2) {
    baz = &foo;
  } else {
    baz = &bar;
  }
  int j = baz(i);
  return j;
}