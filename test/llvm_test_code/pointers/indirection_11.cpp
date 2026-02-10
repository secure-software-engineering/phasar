
class Foo {
public:
  virtual int Func() { return 0; };
};

class Bar : public Foo {
public:
  int Func() override { return 1; };
};

class Baz : public Bar {
public:
  int Func() override { return 2; };
};

class Boar : public Baz {
public:
  int Func() override { return 3; };
};

class Far : public Boar {
public:
  int Func() override { return 4; };
};

int main() {
  Foo First;
  Bar Second;
  Baz Third;
  Boar Fourth;
  Far Fifth;

  int x = First.Func();
  int y = Second.Func();
  int z = Third.Func();
  int a = Fourth.Func();
  int b = Fifth.Func();

  return 0;
}
