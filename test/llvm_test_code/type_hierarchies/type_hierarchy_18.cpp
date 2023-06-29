struct Base {
  virtual int foo() = 0;
  virtual int bar() { return 0; }
};

struct Child : Base {
  int foo() override { return 1; }
  virtual int foobar() = 0;
};

class Child_2 : Child {
  int foobar() override { return 2; }
  virtual int barfoo() = 0;
};

class Child_3 : Child_2 {
  int barfoo() override { return 3; }
};

int main() {
  Child_3 c;
  return 0;
}
