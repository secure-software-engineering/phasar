#include <cmath>
extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  double a = source();
  double b = atan2(a, 1.0);
  sink(b);
  return 0;
}
