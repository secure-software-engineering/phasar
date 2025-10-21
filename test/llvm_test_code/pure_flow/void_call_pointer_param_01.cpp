#include <cstdio>

void call(const int *Integer) { printf("call(%d)\n", *Integer); }

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  call(&One);
  return 0;
}
