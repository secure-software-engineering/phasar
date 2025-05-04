class Base {
  virtual int publicbase() { return 1; }
};

struct Child : public Base {
  int publicbase() override { return 2; }
};

class Foo {
  virtual int protectedfoo() { return 3; }
};

class Bar : protected Foo {
  int protectedfoo() override { return 4; }
};

class Lorem {
  virtual int privatelorem() { return 5; }
};

class Impsum : private Lorem {
  int privatelorem() override { return 6; }
};

int main() {
  Child c;
  Bar b;
  Impsum I;
  return 0;
}
