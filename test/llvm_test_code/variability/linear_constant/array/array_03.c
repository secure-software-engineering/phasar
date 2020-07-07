extern void printChar(char);
extern void puts(const char *);

int main() {
  char bytes[
#ifdef X64
      64
#elif defined(X86)
      32
#else
      1
#endif
  ];

  __builtin_memset(bytes, 0, sizeof(bytes)); // clang intrinsic

  bytes[0] = 'A';

  printChar(bytes[0]);
  printChar(bytes[1]);
  puts(bytes);
}