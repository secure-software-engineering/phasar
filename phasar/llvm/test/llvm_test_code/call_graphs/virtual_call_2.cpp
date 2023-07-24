// handle virtual function call on a super type's reference

struct A {
  virtual ~A() = default;
  virtual void foo() {}
};

struct B : A {
  void foo() override {}
};

int main() {
  B b;
  A &aref = b;
  aref.foo();
}
