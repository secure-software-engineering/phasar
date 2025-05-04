/* immutable: y */
struct A {
  int i;
};

int main() {
  A x;
  x.i = 20;
  x.i = 42;
  A y;
  // y = x;
  return 0;
}
