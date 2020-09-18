// Handle function call of a private function

class Foo {
public:
  void f() { g(); }

private:
  void g() {}
};

int main() {
  Foo foo;
  foo.f();
}