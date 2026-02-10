
class Foo {
public:
  virtual int Func() { return 0; };
};

class Bar : public Foo {
public:
  int Func() override { return 1; };
};

int main() {
  Foo First{};
  Bar Second{};

  int x = First.Func();
  int y = Second.Func();

  return 0;
}
