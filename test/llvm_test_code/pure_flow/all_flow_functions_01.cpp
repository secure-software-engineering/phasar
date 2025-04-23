#include <cstdio>

int call(const int *One, const int Second) {
  int ToReturn = 1;
  printf("call(%d, %d)\nreturn %d", *One, Second, ToReturn);
  return ToReturn;
}

int main(int /*argc*/, char * /*argv*/[]) {
  int One = 1;
  int Two = 2;
  int ReturnInt = call(&One, Two);
  printf("ReturnInt: %d\n", ReturnInt);
  return 0;
}
