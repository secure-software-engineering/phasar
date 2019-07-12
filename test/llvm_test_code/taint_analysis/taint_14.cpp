#include <cstdio>

int doSomething() {
  int tainted = getchar();
  if (tainted < 0)
    return 1;

  printf("I guess, you meant %d\n", tainted + 1);
  return 0;
}

int main() { return doSomething(); }