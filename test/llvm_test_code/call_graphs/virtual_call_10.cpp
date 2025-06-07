// handle virtual function call on a pointer to an interface implementation

struct A {
  virtual ~A() = default;
  virtual void foo() = 0;
};

struct B {
  virtual ~B() = default;
  virtual void bar() = 0;
};

struct ABImpl : A, B {
  void foo() override {}
  void bar() override {}
};

int main() {
  B *ABptr = new ABImpl;
  ABptr->bar();
  delete ABptr;
}
