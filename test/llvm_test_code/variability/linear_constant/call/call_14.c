
#define min(x, y) ((x) > (y) ? (y) : (x))

double compute(int a, ...) {
  double result = a;
  __builtin_va_list va;
  __builtin_va_start(va, a);
  double b;

  b = __builtin_va_arg(va,
#ifdef A
                       double
#else
                       int
#endif
  );
  return min(b, result);
}

int main() {
  int i = 42;
#ifdef A
  double
#else
  int
#endif
      j = i + 1;
  double k = compute(i, j);
  return (int)k;
}