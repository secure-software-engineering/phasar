
typedef struct Pair_t {
  int A;
  int B;
} Pair;

Pair makePair() {
  Pair P;
  P.A = 0;
  P.B = 0;
  return P;
}

__attribute__((annotate("psr.source"))) Pair *taintPair(Pair *P) {
  P->A = 13;
  P->B = 13;
  return P;
}

int main() {
  Pair P = makePair();
  Pair *Q = taintPair(&P);
  return Q->A;
}