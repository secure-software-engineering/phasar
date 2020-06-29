
int compute(int i) {
#ifdef CONFIG
  return ++i;
#else
  return i + 13;
#endif
}

int main() {
  int x = 42;
  x = compute(x);
  return 0;
}
