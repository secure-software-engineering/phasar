#include "source_2.h"

int src2_sixty = 60;

int divide(int a, int b) { return a / b; }

int powers_of_sixty(unsigned a) {
  int result = 1;
  while (a-- > 0) {
    result *= src2_sixty;
  }
  return result;
}
