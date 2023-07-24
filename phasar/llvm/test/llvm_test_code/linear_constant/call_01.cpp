void foo(int a) { // clang-format off
  int b = a;
} // clang-format on

int main() {
  int i = 42;
  foo(i);
  return 0;
}
