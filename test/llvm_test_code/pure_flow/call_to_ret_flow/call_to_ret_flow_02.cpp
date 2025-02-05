#include <cstdio>

int call(int Zero, int One) { return Zero + One; }
void callTwo() {}
int callThree() { return 0; }

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;

  int CallReturn = call(Zero, One);
  callTwo();
  Zero = callThree();

  return Zero;
}
