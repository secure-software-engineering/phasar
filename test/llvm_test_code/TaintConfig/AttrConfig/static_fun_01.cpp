[[clang::annotate("psr.source")]] static int bar(int A, int *B) {
  *B = 13;
  if (A == 42) {
    return 13;
  }
  return A;
}

int main() {
  int A = 42;
  int B = 0;
  int C = bar(A, &B);
  return C;
}
