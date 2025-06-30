#include <cstdio>

int call(const int *Integer) {
  printf("call(%d)\n", *Integer);
  return 1;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int ReturnInt = call(&One);
  printf("ReturnInt: %d\n", ReturnInt);
  return 0;
}
