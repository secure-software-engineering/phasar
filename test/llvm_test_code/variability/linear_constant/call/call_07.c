int foo(int x) { return x + 1; }
int bar(int x) { return x * 2; }

int foobar(int x) {
  return
#ifdef A
      foo
#else
      bar
#endif
      (x + 1);
}

short compute(int x) {
  // Either call a function, or just do some arithmetics
#ifdef B
  return foobar(x);
#else
  return x + 65537;
#endif
}

int main() {
  int i = 10;
  int j = compute(i);
  return j;
}