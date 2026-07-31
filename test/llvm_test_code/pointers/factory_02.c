#include <stdlib.h>

// setPtr writes Val into *Slot: the pointer allocated by makeSlot() escapes
// to another function before makeSlot() returns it.  makeSlot() must NOT be
// classified as a plain allocation wrapper, since giving each call site a
// brand-new, disconnected object would lose the write performed by setPtr.
void setPtr(int **Slot, int *Val) { *Slot = Val; }

int **makeSlot(int *Val) {
  int **Slot = (int **)malloc(sizeof(int *));
  setPtr(Slot, Val);
  return Slot;
}

int main() {
  int X, Y;
  int **P1 = makeSlot(&X);
  int **P2 = makeSlot(&Y);
  int *F1 = *P1;
  int *F2 = *P2;
}
