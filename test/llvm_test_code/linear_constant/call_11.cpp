int bar(int b) {
  return b;
}

int foo(int a) {
  return bar(a);
}

int main() {
  int i;
  i = foo(2);
  return 0;
}