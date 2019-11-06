int g1 = 1;

int foo(int a) {
  a += g1;
  return a;
}

int bar(int b) {
  g1 += 1;
  return b + 1;
}

int baz(int c) {
  return c + 3;
}

int main() {
	g1 += 1;
  int i = 0;
  i = foo(10);
  i = bar(3);
  i = baz(39);
	return 0;
}