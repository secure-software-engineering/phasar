int compute() {
  int k = 0;
  for (
#ifdef REG
      register
#endif
      int i = 0;
      i < 3; ++i) {
    auto int j = i * 2;
    k = j;
  }
  return k;
}

int main() {
  int i = 42;
  int j = compute();
  return i < j ? i : j;
}