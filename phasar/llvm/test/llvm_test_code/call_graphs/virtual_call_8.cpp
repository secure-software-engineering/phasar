// Handle function call (OTF)

int main() {
  struct A;
  struct B;
  struct C;

  static A *a;

  struct A {
  public:
    ~A() = default;
    virtual A *foo() { return nullptr; }
  };

  struct B : public A {
  public:
    ~B() = default;
    virtual A *foo() { return nullptr; }
  };

  struct C : public B {
  public:
    ~C() = default;
    virtual A *foo() override {
      a = new B();
      return a;
    }
  };

  a = new C();
  a->foo();
  a->foo();

  delete a;
}
