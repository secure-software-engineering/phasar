int foo(int a, int b) {
  a += b;
  b += a;
  return a + b;
}

int main() {
  int k;
  k = foo(1, 2);
  return 0;
}
