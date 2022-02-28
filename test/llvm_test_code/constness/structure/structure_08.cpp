/* immutable: x,y */
struct A {
  int i = 42;
};

int main() {
  A x;
  A *y = new A();
  return 0;
}
