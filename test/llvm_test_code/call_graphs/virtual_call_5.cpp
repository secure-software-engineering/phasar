// handle virtual and nonvirtual function call on a super type's pointer

struct A {
public:
  virtual ~A() = default;
  void func() {}
  virtual void Vfunc() {}
};

struct B : public A {
public:
  void func() {}
  void Vfunc() override {}
};

int main() {

  A *a = new B;
  a->func();
  a->Vfunc();

  delete a;
}
