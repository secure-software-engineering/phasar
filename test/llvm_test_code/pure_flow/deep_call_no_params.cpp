#include <cstdio>

int callTwo() {
  printf("callTwo()\n");
  return 2;
}

int call() {
  int ReturnIntCallTwo = callTwo();
  printf("call(%d)\n", ReturnIntCallTwo);
  return 1;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int ReturnInt = call();
  printf("ReturnInt: %d\n", ReturnInt);
  return 0;
}
