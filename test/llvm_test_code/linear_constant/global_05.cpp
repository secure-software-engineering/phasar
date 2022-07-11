int g = 0;

int foo(int a) { // clang-format off
  return ++a;
} // clang-format on

int main() {
  g += 1;
  int i = foo(++g);
  return 0;
}
