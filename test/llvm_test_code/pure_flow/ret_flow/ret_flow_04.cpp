#include <cstdio>

int getTwo() { return 2; }

int call(int ZeroArg, int OneArg) {
  const int TwoInCall = getTwo();
  return ZeroArg + OneArg;
}

int main(int /*argc*/, char * /*argv*/[]) {
  const int Zero = 0;
  const int One = 1;
  const int Two = 2;

  const int CallReturn = call(Zero, One);

  return 0;
}
