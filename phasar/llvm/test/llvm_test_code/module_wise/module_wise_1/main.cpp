#include "src1.h"
#include "src2.h"

int main() {
  int a = 10;
  int b = generate_taint();
  int c = do_computation(a);
  int d = do_computation(b);
  int e = sanitize(c);
  leak_taint(e);
  leak_taint(d);
  return 0;
}
