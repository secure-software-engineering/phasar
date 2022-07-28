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

int main() {
  Child c;
  return 0;
}
