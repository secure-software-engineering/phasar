#include <cstdio>

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;

  int *PtrToOne = &One;
  int **PtrPtrToOne = &PtrToOne;
  int ***PtrPtrPtrToOne = &PtrPtrToOne;

  return 0;
}
