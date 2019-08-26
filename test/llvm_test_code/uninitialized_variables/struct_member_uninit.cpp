struct X {
  short a, b;
  X(short _a) : a(_a) {}
};

short foo(X _x) {
  // return the initialized value
  return _x.a;
}

int main() {
  X x(42);
  foo(x);
  return 0;
}
