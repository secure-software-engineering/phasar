bool Bool = true;

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

int main() {
  Foo First{};
  Bar Second{};
  Baz Third{};

  Foo &FooRef = Bool ? Second : Third;

  return FooRef.Func();
}
