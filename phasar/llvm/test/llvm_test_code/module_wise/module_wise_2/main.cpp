#include "src1.h"

int main(int argc, char **argv) {
  int a = 10 + argc;
  int b = generate_taint();
  int c = a + b;
  int e = sanitize(c);
  int d = e * e;
  leak_taint(d);
  return 0;
}
