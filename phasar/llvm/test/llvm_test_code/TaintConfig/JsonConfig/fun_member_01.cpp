
struct X {
  X(int A, int B) : A(A), B(B) {}
  int foo() const { return A + B + 13; }
  void bar(int &V) const { V = A + B; }

  int A;
  int B;
};

int main() {
  X V(13, 42);
  V.A = 20;
  V.B = 40;
  int C = V.foo();
  int D = 20;
  V.bar(C);
  return V.A + V.B;
}
