int increment(int a) { // clang-format off
   return ++a;
} // clang-format on

int main() {
  int i = increment(42);
  int j = increment(42);
  return 0;
}
