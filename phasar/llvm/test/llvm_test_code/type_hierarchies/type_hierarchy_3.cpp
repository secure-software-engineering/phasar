struct Base {
  virtual int foo() { return 1; }
  virtual int bar() { return -1; }
};

struct Child : Base {
  int foo() override { return 2; };
};

int main() {
  Child c;
  return 0;
}
