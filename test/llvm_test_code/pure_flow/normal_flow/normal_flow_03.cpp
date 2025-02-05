#include <cstdio>

int callTwo(const int *OnePtrArg, const int *FourPtrArg) {
  return *OnePtrArg + *FourPtrArg;
}

void call(const int *OnePtrArg, const int &TwoPtrArg, int ThreeArg) {
  int Four = ThreeArg + 1;
  int Five = callTwo(OnePtrArg, &Four);
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int Two = 2;
  int Three = 3;
  int Zero = 0;

  int *OnePtr = &One;
  int &TwoAddr = Two;

  call(OnePtr, TwoAddr, Three);

  return Zero;
}
