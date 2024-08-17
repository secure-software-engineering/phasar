#include <cmath>
extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
  double a = atan2(4.0, 2.0);
  sink(a);
  return 0;
}
