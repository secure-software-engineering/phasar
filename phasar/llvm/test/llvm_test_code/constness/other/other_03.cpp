/// trick the compiler
static void escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

/// trick the compiler
static void clobber() { asm volatile("" : : : "memory"); }

int main() {
  // i is constant, everything else is not
  int i = 42;
  int j = 13;
  j = 17;
  escape(&i);
  escape(&j);
  return 0;
}
