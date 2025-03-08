#include <cstdio>

void callTwo() {}
void callOne() {}
void call() {
  callOne();
  callTwo();
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  call();
  int Two = 2;
  callOne();
  int Three = 3;

  return 0;
}
