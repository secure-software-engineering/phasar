int foo(int a, int b) { return a + b; }

int bar(int a, int b) { return a * a; }

int (*f)(int, int) = nullptr;

int main() {
  int result;
  f = &foo;
  result = f(1, 2);
  f = &bar;
  result = f(3, 4);
  return 0;
}
