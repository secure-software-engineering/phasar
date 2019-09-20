int foo(int a) {
  return a;
}

int main() {
  int i;
  i = foo(2);
  i = foo(i);
  return 0;
}