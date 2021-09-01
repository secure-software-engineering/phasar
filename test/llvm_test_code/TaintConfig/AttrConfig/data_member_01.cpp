
struct X {
  [[clang::annotate("psr.source")]] int A = 13;
  int B = 0;
};

int main() {
  X V;
  V.B = 42;
  return V.A + V.B;
}
