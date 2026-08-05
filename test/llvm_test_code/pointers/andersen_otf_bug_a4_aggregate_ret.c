// Review item A4: make() returns { ptr, i64 }.  handleReturn populates its
// return slot, but handleCall only binds the call result for pointer-typed
// calls and there is no extractvalue case, so B never learns about A.
struct Pair {
  int *P;
  long N;
};

struct Pair make(int *X) {
  struct Pair R;
  R.P = X;
  R.N = 1;
  return R;
}

int main() {
  int A = 0;
  struct Pair Q = make(&A);
  int *B = Q.P;
  return *B;
}
