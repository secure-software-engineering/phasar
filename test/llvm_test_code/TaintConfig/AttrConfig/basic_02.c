
struct X {
  int A;
  __attribute__((annotate("psr.source"))) int B;
  double C;
};

void modX(struct X *A) {
  A->A = 42;
  A->B = 13;
  A->C = 1.23;
}

int main() {
  struct X A;
  modX(&A);
  return A.B;
}