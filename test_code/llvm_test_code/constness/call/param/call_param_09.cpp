/* immutable: p, i(pointer) */
void foo(int *p) {
  *p = 42;
}

int main() {
  int *i = new int(24);
  foo(i);
  return 0;
}