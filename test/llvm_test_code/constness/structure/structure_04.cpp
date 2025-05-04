/* immutable: x */
struct A {
  int i = 10;
};

int main() {
  A x;
  A y;
  y = x;
  return 0;
}
