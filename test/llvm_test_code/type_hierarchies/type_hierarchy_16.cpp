class Base {
  virtual int foo() { return 1; }
};

struct Child : public Base {
  int foo() override { return 2; }
};

class ChildOfChild : public Child {
  void bar() {}
};

class BaseTwo {
  virtual int foobar() { return 3; }
};

struct ChildTwo : public BaseTwo {
  int foobar() override { return 4; };
};

int main() {
  Child c;
  ChildOfChild o;
  ChildTwo t;
  return 0;
}
