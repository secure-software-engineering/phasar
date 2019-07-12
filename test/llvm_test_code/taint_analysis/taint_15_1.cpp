#include <cstdio>

int doSomething(int argc, char *argv[]) {
  int x = 42;
  int tmp;
  tmp = argc;
  argc = x;
  x = tmp;
  printf("Leak supersecret value: %d\n", x);
  printf("Answer to everything: %d\n", argc);
  return 0;
}
int main(int argc, char *argv[]) { return doSomething(argc, argv); }