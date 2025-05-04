struct X {
  short a, b;
  X() = default;
  X(short _a, short _b) : a(_a), b(_b) {}
};

struct Y : X {
  short z = 0;

  Y() : z(1) {}
};

short foo(X _x) { return _x.a ^ _x.b; }

int main() {
  X x(42, 24);
  foo(x);
  return 0;
}
