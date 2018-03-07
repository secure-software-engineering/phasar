/* immutable: - */
int foo() {
	int a = 42;
  a = 17;
	return a;
}

int main() {
	int i = 10;
	i = foo();
	return 0;
}