#include <cstdio>

void call(int *One) {}
void secondCall(int *One, int **Two, int ***Three) {}

int main(int /*argc*/, char * /*argv*/[]) {

  int One = 1;
  int Two = 2;
  int Three = 3;

  int *PtrToOne = &One;
  int *PtrToTwo = &Two;
  int **PtrPtrToTwo = &PtrToTwo;
  int *PtrToThree = &Three;
  int **PtrPtrToThree = &PtrToThree;
  int ***PtrPtrPtrToThree = &PtrPtrToThree;

  call(PtrToOne);
  secondCall(PtrToOne, PtrPtrToTwo, PtrPtrPtrToThree);

  return 0;
}
