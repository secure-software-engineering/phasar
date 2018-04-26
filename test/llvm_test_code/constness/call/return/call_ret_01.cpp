/* - | - | mem2reg */
int foo() {
	int *a = new int(42);
	return *a;
}

int main() {
	int i = foo();
	return 0;
}