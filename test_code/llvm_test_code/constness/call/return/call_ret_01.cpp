/* mutable: i */
int foo() {
	return 42;
}

int main() {
	int i = 10;
	i = foo();
	return 0;
}