#include <cstdio>

int doSomething(int argc, char *argv[]) {
  int x = 42;
  x ^= argc;
  argc ^= x;
  x ^= argc;
  printf("Leak supersecret value: %d\n", x);
  printf("Answer to everything: %d\n", argc);
  return 0;
}
int main(int argc, char *argv[]) { return doSomething(argc, argv); }
