struct Base {
  virtual ~Base() = default;
  virtual int foo() = 0;
  virtual int bar() { return 0; }
};

struct Base2 {
  virtual ~Base2() = default;
  virtual int foo2() { return 2; }
  virtual int bar2() = 0;
};

struct Base3 {
  virtual ~Base3() = default;
  virtual int foobar() = 0;
};

struct Child : public Base, public Base2 {
  int foo() override { return 10; }
  int bar2() override { return 20; }
};

struct Child2 : public Child, public Base3 {
  int foobar() override { return 30; }
};

int main() {
  Child c;
  Child2 c2;
  return 0;
}
