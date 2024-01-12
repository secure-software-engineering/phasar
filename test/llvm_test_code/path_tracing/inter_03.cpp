int increment(int I) { return ++I; }

int main() {
  int I = 42;
  int J = I;
  int K = increment(J);
  int L = increment(K);
  return L;
}
