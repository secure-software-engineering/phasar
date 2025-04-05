#include <cstdio>

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int Two = 2;
  int *OnePtr = &One;
  int &TwoAddr = Two;
  return 0;
}
