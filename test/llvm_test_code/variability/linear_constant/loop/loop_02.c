
int main() {
  int sum = 0;
#ifdef A
  int c = 12;
  for (int i = 0; i < 10; ++i) {
    sum += c;
  }
#else
  sum = 1000;
#endif
  return 0;
}
