// Handle virtual function call on child and parent type's pointer

struct A {
public:
  virtual ~A() = default;
  virtual void Vfunc() {}
};

struct B : public A {
public:
  virtual ~B() = default;
  void Vfunc() override {}
};

int main() {

  A *a = new A;
  A *b = new B;
  a->Vfunc();
  b->Vfunc();

  delete a, b;
}
