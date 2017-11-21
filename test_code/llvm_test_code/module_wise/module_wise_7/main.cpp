void foo(int &a, int &b, int &c) { c = a + b; }

void bar(int &a) { ++a; }

int main() {
  int a = 1, b = 2, c = 0;
  foo(a, b, c);
  bar(c);
  return 0;
}