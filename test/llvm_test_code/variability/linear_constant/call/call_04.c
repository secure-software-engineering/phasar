
typedef int BOOL;
#define TRUE 1
#define FALSE 0

extern void puts(const char *);

#ifdef X64
BOOL compute(unsigned long long x) { return x & 1; }
#elif defined(X86)
BOOL compute(unsigned int x) { return x & (1 << 31); }
#else
#define compute(x) TRUE
#endif

int main() {
  unsigned y = 2;
  BOOL i = compute(y);
  if (i)
    puts("foo");
  else
    puts("bar");
}