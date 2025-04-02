#include <cstdio>

struct StructOne {
  int One = 1;
};

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int OtherOne = One;
  int MinusOne = !One;
  int Two = One + One;
  int *PtrToOne = &One;
  StructOne ForGEP = StructOne();
  int GEP = ForGEP.One;
  return 0;
}
