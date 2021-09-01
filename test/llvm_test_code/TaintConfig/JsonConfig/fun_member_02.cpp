#include <cstdio>

struct X {
  X(int V) : V(V) {}
  X(const X &) = default;
  X &operator=(const X &) = default;
  ~X() { printf("V is: %d\n", V); }
  void sanit() { printf("V is: %d\n", V); }
  static int returnMagic() { return 42; }

  int V;
};

int main() {
  X V(13);
  X W = V;
  X Y(X::returnMagic());
  Y.sanit();
  return W.V;
}
