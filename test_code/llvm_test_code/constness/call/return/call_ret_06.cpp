/* immutable: i */
int* foo() {
  int a = 42;
  // a = 17;
  return &a;
}

int* bar() {
  int *b = foo();
  return b;
}

int main() {
  int *i;
  i = bar();
  *i = 20;
  return 0;
}