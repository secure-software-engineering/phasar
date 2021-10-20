#include <stdlib.h>

int main() {
  char *Buffer = (char *)malloc(128 * sizeof(char));
  for (int I = 0; I < 128; ++I) {
    Buffer[I] = 42;
  }
  Buffer[42] = 13;
  __attribute__((annotate("psr.source"))) char *P = &Buffer[42];
  free(Buffer);
  return 0;
}