#include <stdlib.h>

void doFree(void *P) { free(P); }

int main() {
  void *X = malloc(32);
  doFree(X);
  free(X);
}
