#include <stdbool.h>
#include <stdio.h>

#ifdef X64
bool compute(unsigned long long x) { return x & 1; }
#elif defined(X86)
bool compute(unsigned int x) { return x & (1 << 31); }
#else
#define compute(x) true
#endif

int main() {
  if (compute(2))
    puts("foo");
  else
    puts("bar");
}