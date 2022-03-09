struct Base {
  virtual int foo() { return 0; }
  virtual int bar() { return 1; }
};

struct Child : Base {
  int foo() override { return 2; }
  virtual int baz() { return -17; }
};

class NonvirtualClass {
public:
  int tar() { return 42; }
};

struct NonvirtualStruct {
  int j;
};

int main() {
  Child c;
  NonvirtualClass nc;
  nc.tar();
  NonvirtualStruct ns;
  ns.j = 200;
  return 0;
}
