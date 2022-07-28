#include <cstdio>
int &foo(int &x, int n) {
  if (n < 1)
    return x;
  return foo(x, n - 1);
}

int main() {
  int i;
  int j = foo(i, 10);
  printf("%d\n", j);
  return 0;
}
