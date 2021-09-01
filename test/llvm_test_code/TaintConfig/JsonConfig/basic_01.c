#include <stdarg.h>

int foo(int A, int B) { return 0; }

int bar() { return 0; }

int baz(int A, int B, int C) { return 0; }

void quark() {}

double average(int Count, ...) {
  va_list Args;
  double Tot = 0;
  va_start(Args, Count);
  for (int J = 0; J < Count; J++) {
    Tot += va_arg(Args, double);
  }
  va_end(Args);
  return Tot / Count;
}

int main() {
  int A = foo(42, 13);
  int B = bar();
  int C = A + 42;
  C = baz(C, A, B);
  quark();
  double Avg = average(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  return C;
}