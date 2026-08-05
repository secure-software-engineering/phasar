// Review item A4 (atomics), in the shape clang actually emits: a pointer
// exchange is lowered to `atomicrmw xchg ptr %P, i64 ...`, so the operation
// is dropped twice over -- processInstruction has no AtomicRMWInst case, and
// the i64-punned value chain is skipped by definitelyContainsNoPointer.
// Old therefore aliases nothing, and B never reaches P's object.
int main() {
  int A = 0;
  int B = 0;
  int *P = &A;
  int *Q = &B;
  int *Old = __atomic_exchange_n(&P, Q, __ATOMIC_SEQ_CST);
  int *Cur = P;
  int *U = Old;
  int *V = Cur;
  return *U + *V;
}
