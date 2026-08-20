// A pointer-sized integer view of a pointer-holding slot. Both the store and
// the load are typed i64, so the plain integer-type gate would drop the whole
// chain and Q would alias nothing.
#include <stdint.h>

int A;
int B;

int main() {
  int *P = &A;
  *(intptr_t *)&P = (intptr_t)&B;
  int *Q = (int *)*(intptr_t *)&P;
  return *Q;
}
