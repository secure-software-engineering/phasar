#include "src1.h"

void foo(int *i_ptr) {
  *i_ptr = 13;
  bar(i_ptr);
}

void bar(int *i_ptr) { *i_ptr = 69; }
