struct Base {
  virtual int foo() = 0;
  virtual int bar() { return 0; }
};

struct Base2 {
  virtual int foo2() { return 2; }
  virtual int bar2() = 0;
};

struct Child : public Base, public Base2 {
  int foo() override { return 10; }
  int bar2() override { return 20; }
};

int main() {
  Child c;
  return 0;
}
