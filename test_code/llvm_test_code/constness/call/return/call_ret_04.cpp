/* mutable: ret value of foo */
int* foo() {
	return new int(42);
}

int main() {
	int *p = foo();
	*p = 13;
	return 0;
}