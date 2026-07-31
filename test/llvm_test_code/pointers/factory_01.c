
#include <stdio.h>
#include <stdlib.h>

void *factory_fun(size_t Sz) {
  void *Mem = malloc(Sz);
  if (!Mem) {
    fputs("bad_alloc", stderr);
    exit(1);
  }

  return Mem;
}

extern void ASSERT_NOALIAS(void *, void *);

int main() {
  void *P1 = factory_fun(42);
  void *P2 = factory_fun(42);
  ASSERT_NOALIAS(P1, P2);
}
