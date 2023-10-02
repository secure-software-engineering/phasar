#include <stdlib.h>

int main() {
  void *X = malloc(32);
  free(X);
  free(X);
}
