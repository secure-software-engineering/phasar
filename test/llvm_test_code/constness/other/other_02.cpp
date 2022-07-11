/// trick the compiler
static void escape(void *p) { asm volatile("" : : "g"(p) : "memory"); }

/// trick the compiler
static void clobber() { asm volatile("" : : : "memory"); }

int main() {
  // i is not constant
  int i = 42;
  i = 13;
  escape(&i);
  return 0;
}
