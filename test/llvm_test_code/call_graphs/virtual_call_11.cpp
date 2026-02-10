// handle virtual function call on a pointer to an interface implementation

struct A {
  virtual void foo() = 0;
};

struct B {
  virtual void bar() = 0;
};

struct ABImpl : A, B {
  void foo() override {}
  void bar() override {}
};

struct C {

  virtual void baz() {}
};

struct ABCImpl : C, ABImpl {
  void foo() override {}
  void bar() override {}
  void baz() override {}
};

void callFoo(A &a) { //
  a.foo();
}

void callBar(B &b) { //
  b.bar();
}

void callBaz(C &c) { //
  c.baz();
}

int main() {
  ABCImpl abc;

  callFoo(abc);
  callBar(abc);
  callBaz(abc);
}
