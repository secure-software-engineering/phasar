#include <cstdio>

int call(int Integer) {
  printf("call(%d)\n", Integer);
  return 1;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int ReturnInt = call(1);
  printf("ReturnInt: %d\n", ReturnInt);
  return 0;
}
