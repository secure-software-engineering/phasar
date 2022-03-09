/// trick the compiler
static void escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

/// trick the compiler
static void clobber() { asm volatile("" : : : "memory"); }

int main() {
  // i is constant
  int i = 42;
  escape(&i);
  return 0;
}
