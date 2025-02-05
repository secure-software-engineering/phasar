#include <cstdio>

int getOne() { return 1; }
int getTwo(int One) { return One + 1; }
int getThree(const int *Two) { return *Two + 1; }

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;
  int Two = 2;

  One = getOne();
  Two = getTwo(One);
  int Three = getThree(&Two);

  return Zero;
}
