int bar(int b) { // clang-format off
  return b;
}

int foo(int a) {
  return bar(a);
} // clang-format on

int main() {
  int i;
  i = foo(2);
  return 0;
}
