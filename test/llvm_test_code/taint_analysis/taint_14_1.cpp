#include <cstdio>

int main() {
  int tainted = getchar();
  if (tainted < 0)
    return 1;

  printf("I guess, you meant %d\n", tainted + 1);
  return 0;
}
