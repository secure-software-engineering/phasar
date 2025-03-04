#include <cstdio>

int GlobalFour = 4;

int getOne() { return 1; }
int getTwo(int One) { return One + 1; }
int getThree(const int *Two) { return *Two + 1; }
int *getPtrToGlobalFour() { return &GlobalFour; }

int main(int /*argc*/, char * /*argv*/[]) {

  int One = getOne();
  int Two = getTwo(One);
  int Three = getThree(&Two);
  int *PtrToGlobalFour = getPtrToGlobalFour();

  return 0;
}
