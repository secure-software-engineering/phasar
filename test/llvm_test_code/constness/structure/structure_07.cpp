/* immutable: y */
struct A {
  int i;
};

int main() {
  A x;
  x.i = 20;
  A *y;
  y = &x;
  y->i = 42;
  return 0;
}
