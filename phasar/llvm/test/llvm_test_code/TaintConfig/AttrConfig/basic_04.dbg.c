int main() {
  int I = 42;
  __attribute__((annotate("psr.source"))) int J = 13;
  int K = I + J;
  return K;
}
