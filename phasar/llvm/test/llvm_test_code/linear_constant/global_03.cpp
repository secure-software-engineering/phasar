int g = 0;

void foo() { // clang-format off
  g += 1;
} // clang-format on

int main() {
  int i = 42;
  g += 1;
  foo();
  return 0;
}
