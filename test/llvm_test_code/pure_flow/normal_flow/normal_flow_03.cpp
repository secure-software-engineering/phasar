#include <cstdio>

void call() {}

void callTwo() {}

void callThree() {}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  call();
  int One = 1;
  callTwo();
  int Two = 2;
  callThree();
  int Three = 3;

  return 0;
}
