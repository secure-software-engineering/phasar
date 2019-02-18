// Handle polymorphic function call (CHA and RTA)
#include <cstdlib>
#include <ctime>

struct A {
public:
  virtual ~A() = default;
  virtual void foo() {}
};

struct B : public A {
public:
  virtual ~B() = default;
  virtual void foo() override {}
};

struct C : public A {
public:
  virtual ~C() = default;
  virtual void foo() override {}
};

struct D : public A {
public:
  virtual ~D() = default;
  virtual void foo() override {}
};

struct F : public A {
public:
  virtual ~F() = default;
  virtual void foo() override {}
};

A *createObj() {
  srand((int)time(0));
  int i = rand();

  switch (i % 3) {
  case 0: {
    return new B;
    break;
  }
  case 1: {
    return new C;
    break;
  }
  case 2: {
    return new D;
    break;
  }
  }
}

int main() {
  A *a = createObj();
  a->foo();
  delete a;
}
