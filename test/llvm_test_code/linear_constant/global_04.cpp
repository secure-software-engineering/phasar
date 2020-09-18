int g = 0;

int foo(int a) {
  return ++a;
}

int main() {
	g += 1;
  int i = foo(g);
	return 0;
}