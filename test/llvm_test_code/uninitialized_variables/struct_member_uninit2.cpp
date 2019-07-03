struct X {
  short a, b;
  X(short _a) : a(_a) {}
};

short foo(X _x) {
  // return the uninitialized value
  return _x.b;
}

int main() {
  X x(42);
  foo(x);
  return 0;
}
