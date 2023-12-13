int increment(int I) { return ++I; }

int compute(int I) { return (I + 42) * 2; }

int main() {
  int I = 42;
  int J = I;
  int K = compute(J);
  K = increment(K);
  return K;
}
