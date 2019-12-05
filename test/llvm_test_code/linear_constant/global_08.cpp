int g = 1;

int baz(int c) {
  return g + c;
}

int bar(int b) {
  return baz(b + 1);
}

int foo(int a) {
  return bar(a + 1);
}

int main() {
  g += 1;
  int i = 0;
  i = foo(1);
	return 0;
}