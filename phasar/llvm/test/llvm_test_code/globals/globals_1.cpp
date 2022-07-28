int globalInt = 10;

struct Base {
  virtual int foo() { return globalInt; }
};

struct Child : Base {
  int foo() override { return globalInt + 10; }
};

int main() {
  Child c;
  int i = c.foo();
  return 0;
}
