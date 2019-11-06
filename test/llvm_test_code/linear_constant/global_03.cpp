int g = 0;

void foo() {
  g += 1;
}

int main() {
  int i = 42;
	g += 1;
  foo();
	return 0;
}