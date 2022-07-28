struct Base {
  virtual void foo() {}
};

struct Derived : Base {
  void foo() override {}
};

int main() {
  Derived d;
  Base &b = d;
  b.foo();
  return 0;
}
