int increment(int a) { // clang-format off
  return ++a;
} // clang-format on

int main() {
  int i = 42;
  int j = increment(i);
  int k = increment(j);
  return 0;
}
