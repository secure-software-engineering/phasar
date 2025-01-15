#include <cstdio>

int call() {
  printf("call()\n");
  return 1;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int ReturnInt = call();
  printf("ReturnInt: %d\n", ReturnInt);
  return 0;
}
