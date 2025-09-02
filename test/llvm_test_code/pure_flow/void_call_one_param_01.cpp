#include <cstdio>

void call(int Integer) { printf("call(%d)\n", Integer); }

int main(int /*argc*/, char * /*argv*/[]) {
  call(1);
  return 0;
}
