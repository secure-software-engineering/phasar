/* mutable: i */
void foo() {
	int a = 42;
}

int main() {
	int i = 13;
	i += 42;
	foo();
	return 0;
}