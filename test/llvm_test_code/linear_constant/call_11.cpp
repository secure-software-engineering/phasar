int foo(int a) {
  return 42;
}

int main() {
  int i;
  i = foo(2);
  i = foo(i);
  return 0;
}