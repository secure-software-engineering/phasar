int g = 0;

int foo(int a) {
  a += g;
  return a;
}

int bar(int b) {
  g += 1;
  return b + 1;
}

int main() {
	g += 1;
  int i = 0;
  i = foo(10);
  i = bar(3);
	return 0;
}