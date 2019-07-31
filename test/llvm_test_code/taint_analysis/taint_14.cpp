#include <cstdio>

int doSomething() {
  int tainted = getchar();
  if (tainted < 0)
    return 1;

  printf("Leak super secret value: %d\n", tainted);
  return 0;
}

int main() { return doSomething(); }