int id(int i) { return i; }

int inc(int i) { return ++i; }

int add(int i, int j) { return i + j; }

int main() {
  int a = 0;
  int b = 1;
  a = inc(a);
  b = id(b);
  int c = add(a, b);
  return 0;
}
