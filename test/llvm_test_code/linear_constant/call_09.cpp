int increment(int i) { return ++i; }

int main() {
  int i = 42;
  while(i > 0) {
    i = increment(i);
  }
  int a = i;
  return 0;
}