struct Base {
  virtual int foo() { return 1; }
  virtual int bar() { return -1; }
  int other() { return -10; }
};

struct OtherBase {
  virtual int baz() { return 42; }
  int othersother() { return -1000; }
};

struct Child : Base, OtherBase {
  int foo() override { return 2; }
  int baz() override { return 400; }
  virtual int tar() { return 100; }
};

int main() {
  Child c;
  return 0;
}
