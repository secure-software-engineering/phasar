/* immutable: - */
void foo() {
	int a = 0;
	a += 10;
}

int main() {
	int i = 20;
	i += 42;
	foo();
	return 0;
}