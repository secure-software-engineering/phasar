#include <cstdio>

void call(int Zero, int One) {}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  call(Zero, One);
  return Zero;
}
