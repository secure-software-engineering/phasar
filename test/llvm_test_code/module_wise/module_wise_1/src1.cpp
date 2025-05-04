#include "src1.h"

int generate_taint() {
  // 13 is an evil tainted value
  return 13;
}

int do_computation(int i) {
  int a = 10 + i;
  int b = a * a * 13;
  return b;
}
