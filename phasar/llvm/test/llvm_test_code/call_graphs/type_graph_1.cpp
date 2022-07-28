// Handle function call (VTA and DTA)
struct A {
public:
  ~A() = default;
};

struct B : public A {
public:
  ~B() = default;
};

int main() {
  A *a1 = new A();
  A *a2 = new A();
  A *a3;
  B *b1;

  a1 = new A();
  a2 = new A();
  a1 = a2;
  a3 = a1;
  a3 = b1;
  b1 = (B *)a3;

  delete a1, a2, a3, b1;
}
