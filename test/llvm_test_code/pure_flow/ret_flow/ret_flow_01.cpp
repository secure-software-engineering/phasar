#include <cstdio>

int getTwo() { return 2; }

int call(int ZeroArg, int OneArg) {
  int TwoInCall = getTwo();
  return ZeroArg + OneArg;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  int Two = 2;

  int CallReturn = call(Zero, One);

  return 0;
}
