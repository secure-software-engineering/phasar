struct Base {
  virtual int foo() = 0;
  virtual int bar() { return 1; }

  int i;
};

struct Child : Base {
  int foo() override { return 2; }
  virtual int baz() { return -17; }
  int j;
};

struct Child2 : Base {
  int foo() override { return 7; }
  virtual int dev() { return -9; }
  int j;
  int k = -1;
};

struct Base2 {
  virtual int foo() = 0;
  virtual int bar() { return 1; }
  virtual int barfoo() = 0;
  virtual int foobar() { return 2; }

  int i = 2;
};

struct Kid : Base2 {
  int foo() override { return 2; }
  virtual int bau() { return 11; }
  int barfoo() override { return 521; }
  int j;
};

int main() {
  Child c;
  Kid k;
  return 0;
}
