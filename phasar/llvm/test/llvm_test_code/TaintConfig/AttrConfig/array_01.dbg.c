
int main() {
  char Buffer[128];
  for (int I = 0; I < 128; ++I) {
    Buffer[I] = 42;
  }
  Buffer[42] = 13;
  __attribute__((annotate("psr.source"))) char *P = &Buffer[42];
  return 0;
}
