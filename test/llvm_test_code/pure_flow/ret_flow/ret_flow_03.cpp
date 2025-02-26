#include <cstdio>

int GlobalFour = 4;

int getTwo() { return 2; }

const int *newThree(const int *Three) {
  const int *NewThreePtr = &(*Three);
  return NewThreePtr;
}

int *getFourPtr() { return &GlobalFour; }

int &getFourAddr() { return GlobalFour; }

int call(int Zero, const int *One) {
  int Two = getTwo();
  int Three = 3;
  int *ThreePtr = &Three;
  const int *NewThree = newThree(ThreePtr);

  return Zero + *One + Two + *NewThree + *getFourPtr() + getFourAddr();
}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;

  int CallReturn = call(Zero, &One);
  CallReturn = 0;

  return CallReturn;
}
