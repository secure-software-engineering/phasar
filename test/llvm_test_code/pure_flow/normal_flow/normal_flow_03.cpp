#include <cstdio>

void call() { int Three = 3; }

void callTwo() { int Four = 4; }

int main(int /*argc*/, char * /*argv*/[]) {
  int Zero = 0;
  call();
  int One = 1;
  callTwo();
  int Two = 2;

  return 0;
}
