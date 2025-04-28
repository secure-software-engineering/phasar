#include <cstdio>

int call(int Zero, int One) { return Zero + One; }

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;

  int CallReturn = call(Zero, One);

  return 0;
}
