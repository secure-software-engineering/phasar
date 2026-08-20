// Check that the analysis handles atomic operations properly; Note: Clang
// treats the Q argument below as i64, not as ptr
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
