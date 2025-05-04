int foo() { // clang-format off
   return 42;
} // clang-format on

int main() {
  int i = 10;
  foo();
  return 0;
}
