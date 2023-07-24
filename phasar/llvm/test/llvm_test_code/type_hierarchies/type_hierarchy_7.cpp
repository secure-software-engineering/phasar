struct A {
  virtual int f() { return 0; }
};
struct B : A {};
struct D : B {};
struct C : A {};
struct X {
  virtual int g() { return 1; }
};
struct Y : X {};
struct Z : C, Y {};

int main() {
  Z z;
  D d;
  return 0;
}
