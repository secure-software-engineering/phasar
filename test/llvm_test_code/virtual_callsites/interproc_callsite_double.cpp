struct Base {
  virtual void foo() {}
  virtual int bar() { return 1; }
};

struct Derived : Base {
  void foo() override {}
  int bar() override { return 2; }
};

void callFunction(Base &b) { int i = b.bar(); }

int main() {
  Derived d;
  callFunction(d);
  Base b;
  callFunction(b);
  return 0;
}
