#include "src1.h"

void foo(MyStruct &s) {
  s.i = 42;
  bar(s);
}

void bar(MyStruct &s) { s.i = 69; }
