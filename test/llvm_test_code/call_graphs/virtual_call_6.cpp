// handle virtual function call on super and child objects

struct A {
public:
  virtual ~A() = default;
  virtual void Vfunc() {}
};

struct B : public A {
public:
  virtual ~B() = default;
  virtual void Vfunc() override {}
};

int main() {

  A a;
  a.Vfunc();
  B b;
  b.Vfunc();
}
