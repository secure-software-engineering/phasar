int increment(int a) { // clang-format off
   return ++a;
} // clang-format on

int main() {
  int i = 42;
  i = increment(i);
  return 0;
}
