//#include <stdarg.h>

int compute(int a
#ifdef VAA
            ,
            ...
#else
            ,
            int b
#endif
) {
#ifdef VAA
  __builtin_va_list va;
  __builtin_va_start(va, a);

  int other = __builtin_va_arg(va, int);

  return other;
#else
  return a;
#endif
}

int main() {
  int i = 43;
  int j = 44;

  int k = compute(i, j);

  return j + 1;
}