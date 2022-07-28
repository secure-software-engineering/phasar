class Child {
  virtual int foo();
};

struct ChildsChild : public Child {};

void user() { ChildsChild c; }
