
class Foo {
public:
  virtual int Func() { return 0; };
};

class Bar : public Foo {
public:
  Foo *FooPtr;
  int Func() override { return 1; };
};

class Baz : public Bar {
public:
  Foo *FooPtr;
  int Func() override { return FooPtr->Func(); };
};

int main() {
  Foo First{};
  Bar Second{};

  Foo &SecondAlias = Second;

  SecondAlias.Func();

  return 0;
}
