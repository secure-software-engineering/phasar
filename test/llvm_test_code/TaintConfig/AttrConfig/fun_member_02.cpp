#include <cstdio>

struct X {
  X(int V) : V(V) {}
  X(const X &) = default;
  X &operator=(const X &) = default;
  [[clang::annotate("psr.sink")]] ~X() { printf("V is: %d\n", V); }
  [[clang::annotate("psr.sanitizer")]] void sanit() { printf("V is: %d\n", V); }
  static int returnMagic() { return 42; }

  [[clang::annotate("psr.source")]] int V;
};

int main() {
  X V(13);
  X W = V;
  X Y(X::returnMagic());
  Y.sanit();
  return W.V;
}