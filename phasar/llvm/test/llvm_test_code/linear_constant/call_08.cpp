int foo(int a, int b) { // clang-format off
  return a + b;
} // clang-format on

int main() {
  int i = 10;
  int j = 1;
  int k;
  k = foo(i, j);
  return 0;
}
