#include <cstdio>

int getTwo() { return 2; }

int newThree() {
  int Four = 4;
  return 3;
}

int call(int Zero, int One) {
  int Two = getTwo();
  int Three = 3;

  Three = newThree();

  return Zero + One;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  int One = 1;

  int CallReturn = call(Zero, One);

  return Zero;
}
