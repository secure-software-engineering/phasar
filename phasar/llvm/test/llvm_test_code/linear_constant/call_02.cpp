int foo(int a) { // clang-format off
  return a + 40;
} // clang-format on

int main() {
  int i;
  i = foo(2);
  return 0;
}
