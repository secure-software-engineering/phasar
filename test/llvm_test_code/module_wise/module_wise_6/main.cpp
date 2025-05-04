#include "src1.h"
#include "src2.h"

void inc(int &i) { ++i; }

int main() {
  int i = 42;
  foo(i);
  bar(i);
  inc(i);
  return 0;
}
