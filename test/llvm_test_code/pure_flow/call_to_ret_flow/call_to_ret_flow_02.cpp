#include <cstdio>

void callTwo() {}
void callOne() {}
void call() {
  int Three = 3;
  callOne();
  int Four = 4;
  callTwo();
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  call();
  int Two = 2;
  callOne();

  return 0;
}
