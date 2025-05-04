void bar(int b) {}

void foo(int a) { // clang-format off
  bar(a);
} // clang-format on

int main() {
  int i;
  foo(2);
  return 0;
}
