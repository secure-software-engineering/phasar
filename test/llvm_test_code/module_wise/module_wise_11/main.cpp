int *foo(int *a) {
  *a = 42;
  return a;
}

int main() {
  int a = 10;
  int *b = foo(&a);
  return 0;
}
