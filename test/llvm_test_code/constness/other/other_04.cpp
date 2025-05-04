/// trick the compiler
static void escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

/// trick the compiler
static void clobber() { asm volatile("" : : : "memory"); }

int main() {
  // i is constant, everything else is not
  int i = 42;
  int j = 1;
  int k = 2;
  j = j + i;
  k = k + i;
  int l = k * k;
  escape(&i);
  escape(&j);
  escape(&k);
  escape(&l);
  return 0;
}
