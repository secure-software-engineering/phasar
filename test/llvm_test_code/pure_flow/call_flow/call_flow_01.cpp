#include <cstdio>

void call(int ArgZero, int ArgOne) {}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  call(Zero, One);
  return 0;
}
