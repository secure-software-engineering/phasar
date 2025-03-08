#include <cstdio>

int GlobalFour = 4;

const int *newThree(const int *ThreeArg) {
  const int *NewThreePtrInFunc = &(*ThreeArg);
  return NewThreePtrInFunc;
}

int *getFourPtr() { return &GlobalFour; }

int &getFourAddr() { return GlobalFour; }

int call(int &ZeroArg, const int *OneArg) {
  int TwoInCall = 2;
  int ThreeInCall = 3;
  int *ThreePtrInCall = &ThreeInCall;
  const int *NewThreeInCall = newThree(ThreePtrInCall);

  return ZeroArg + *OneArg + TwoInCall + *NewThreeInCall + *getFourPtr() +
         getFourAddr();
}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  int Two = 2;

  int CallReturn = call(Zero, &One);

  return 0;
}
