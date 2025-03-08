#include <cstdio>

void call(int ZeroArg, int OneArg) {}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  int Two = 2;

  call(Zero, One);
  return 0;
}
