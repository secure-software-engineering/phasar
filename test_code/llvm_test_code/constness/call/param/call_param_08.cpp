/* immutable: p */
void foo(int *p) {
  *p = 42;
}

int main() {
  int i;
  foo(&i);
  foo(&i);
  foo(&i);
  return 0;
}