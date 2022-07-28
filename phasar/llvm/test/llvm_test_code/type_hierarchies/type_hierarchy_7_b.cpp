struct A {
  virtual int f();
};
struct C : A {};
struct X {
  virtual int g();
};
struct Y : X {};
struct Z : C, Y {};
class Omega : Z {
  int f() override { return -1000; }
};

void user() { Omega o; }
