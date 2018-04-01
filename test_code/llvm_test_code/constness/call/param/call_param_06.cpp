/* - | - | mem2reg */
void foo(int *p) {
  int b = *p;
}

int main() {
  int *i = new int(24);
  foo(i);
  return 0;
}