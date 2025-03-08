#include <cstdio>

int getTwo() { return 2; }

int newThree() {
  int FourInFunc = 4;
  return 3;
}

int call(int ZeroArg, int OneArg) {
  int TwoInCall = getTwo();
  int ThreeInCall = 3;

  ThreeInCall = newThree();

  return ZeroArg + OneArg;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  int Two = 2;

  int CallReturn = call(Zero, One);

  return 0;
}
