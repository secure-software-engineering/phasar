#include <cstdio>

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;

  int *PtrToOne = &One;
  int **PtrPtrToOne = &PtrToOne;
  int ***PtrPtrPtrToOne = &PtrPtrToOne;

  int Deref1 = *PtrToOne;
  int Deref2 = **PtrPtrToOne;
  int Deref3 = ***PtrPtrPtrToOne;

  return 0;
}
