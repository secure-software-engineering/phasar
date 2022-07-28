// Handle function call on parent and child type's pointers

struct A {
public:
  ~A() = default;
  void Vfunc() {}
};

struct B : public A {
public:
  ~B() = default;
  void Vfunc() {}
};

int main() {

  A *a1 = new A;
  a1->Vfunc();

  B *b = new B;
  b->Vfunc();

  A *a2 = b;
  a2->Vfunc();

  delete a1, b, a2;
}
