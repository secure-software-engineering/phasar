#include <cstdio>

void call(int ThreeArg, const int *TwoPtrArg) {
  int Five = ThreeArg + *TwoPtrArg;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int Two = 2;

  {
    int *OnePtr = &One;
    int &TwoAddr = Two;
  }

  int Three = 3;

  call(Three, &Two);

  return 0;
}
