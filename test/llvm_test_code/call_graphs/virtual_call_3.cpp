// handle virtual function call on a pointer to an interface implementation

struct A {
  virtual ~A() = default;
  virtual void foo() = 0;
};

struct AImpl : A {
  void foo() override {}
};

int main() {
  A *aptr = new AImpl;
  aptr->foo();
  delete aptr;
}
