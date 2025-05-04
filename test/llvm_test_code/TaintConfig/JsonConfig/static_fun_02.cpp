static int bar(int A, int *B) {
  *B = 13;
  if (A == 42) {
    return 13;
  }
  return A;
}

namespace {
void foo(int &A) { A = 13; }
} // anonymous namespace

int main() {
  int A = 42;
  int B = 0;
  int C = bar(A, &B);
  foo(C);
  return C;
}
