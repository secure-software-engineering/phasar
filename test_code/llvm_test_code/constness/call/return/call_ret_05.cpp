/* immutable: p */
int* foo() {
	return new int(42);
}

int main() {
	int *p = foo();
	*p = 13;
	return 0;
}