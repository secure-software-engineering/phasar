#include <cstdio>

class A {
public:
  virtual ~A() = default;

  virtual void foo() = 0;
};

class B : public A {
public:
  virtual void foo() override { //
    puts("Calling B::foo()");
  }
};

class C : public A {
public:
  virtual void foo() override { //
    puts("Calling C::foo()");
  }
};

int main() {
  B b;
  b.foo();

  C c;
  c.foo();

  A &a = b;
  a.foo();
}
