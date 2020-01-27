int g = 0;

int foo() {
  return ++g;
}

int main() {
	g += 1;
  int i = foo();
	return 0;
}