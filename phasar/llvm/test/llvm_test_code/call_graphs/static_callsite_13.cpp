// Handle function call on parent and child type's object passed as argument

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

void Vfunc(A *a) { a->Vfunc(); }

int main() {

  A *a = new A;
  Vfunc(a);

  B *b = new B;
  Vfunc(b);

  delete a, b;
}
