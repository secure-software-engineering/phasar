int g = 0;

int foo() { // clang-format off
  return ++g;
} // clang-format on

int main() {
  g += 1;
  int i = foo();
  return 0;
}
