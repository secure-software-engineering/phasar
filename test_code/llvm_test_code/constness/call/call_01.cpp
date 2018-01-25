/* mutable: a */
void foo() {
	int a = 0;
	a += 42;
}

int main() {
	int i = 13;
	foo();
	return 0;
}