class Base {
  virtual int foo();
};

struct Child : public Base {
  virtual int foo();
};

struct ChildsChild : Child {
  int foo() override { return 2; }
};

void user() { ChildsChild c; }
