#include <cstdio>

int GlobalFour = 4;

int getOne() { return 1; }
int getTwo(int OneArg) { return OneArg + 1; }
int getThree(const int *TwoArg) { return *TwoArg + 1; }
int *getPtrToGlobalFour() { return &GlobalFour; }

int main(int /*argc*/, char * /*argv*/[]) {

  int Zero = 0;
  int One = getOne();
  int Two = getTwo(One);
  int Three = getThree(&Two);
  int *PtrToGlobalFour = getPtrToGlobalFour();

  return 0;
}
