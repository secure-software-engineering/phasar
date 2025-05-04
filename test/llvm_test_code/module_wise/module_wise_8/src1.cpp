#include "src1.h"

void foo(int &a, int &b, int &c) {
  c = a + b;
  inc(c);
}

void inc(int &a) { ++a; }
