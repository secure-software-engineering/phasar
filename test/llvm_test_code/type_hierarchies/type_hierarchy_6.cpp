struct Base {
  virtual int foo() { return 1; }
  virtual int bar() { return -1; }
  int other() { return -10; }
};

struct Child : Base {
  int foo() override { return 2; }
  virtual int tar() { return 100; }
};

int main() {
  Child c;
  return 0;
}
