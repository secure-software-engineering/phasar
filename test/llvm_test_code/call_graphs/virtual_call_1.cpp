// handle simple virtual function call

struct A {
  virtual ~A() = default;
  virtual void foo() {}
};

int main() {
  A a;
  a.foo();
}
