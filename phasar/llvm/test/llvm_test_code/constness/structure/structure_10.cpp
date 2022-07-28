/* immutable: x */
class A {
private:
  int i = 42;

public:
  void foo(int p) { i = p; }
};

int main() {
  A x;
  x.foo(17);
  return 0;
}
