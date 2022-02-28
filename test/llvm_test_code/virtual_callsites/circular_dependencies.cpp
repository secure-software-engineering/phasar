struct A {
  virtual void foo();
};
struct B : A {
  void foo() override;
};
struct C : A {
  void foo() override;
};

A *field = nullptr;

void A::foo() { field = new B; }
void B::foo() { field = new C; }
void C::foo() {
  // do nothing
}

int main() {
  field = new A();
  field->foo();
  return 0;
}
