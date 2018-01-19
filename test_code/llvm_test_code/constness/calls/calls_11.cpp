/* mutable: a */
int foo() {
	return 42;
}

int main() {
	int a = 10;
	a = foo();
	return 0;
}