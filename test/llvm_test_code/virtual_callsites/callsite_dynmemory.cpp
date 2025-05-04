struct Base {
  virtual void foo() {}
};

struct Derived : Base {
  void foo() override {}
};

int main() {
  Base *b = new Derived;
  b->foo();
  delete b;
  return 0;
}
