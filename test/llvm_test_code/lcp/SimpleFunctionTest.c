int foo(int x) { return x; }
int bar(int x) { return x * 2; }
int main() {
  int i = 42;
  int j = 24;
  i = foo(i);
  j = foo(j);
  i = bar(foo(j));
  j = foo(i + j);
  return i;
}