#include "src1.h"

void make_call(Abstract &A) {
  int i = 42;
  A.foo(i);
}

int main() {
  Abstract *A = give_me();
  make_call(*A);
  return 0;
}
