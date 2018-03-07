/* immutable: a */
int foo() {
	int a = 42;
	return a;
}

int main() {
	int i = 10;
	i = foo();
	return 0;
}