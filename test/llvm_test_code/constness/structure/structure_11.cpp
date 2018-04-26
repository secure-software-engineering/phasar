/* immutable: x */
class A {
private:
  int i;
public:
  A(int a) : i(a) {}
  void foo(int p) { i = p; }
};

int main() {
  A x(42);
  x.foo(17);
  return 0;
}