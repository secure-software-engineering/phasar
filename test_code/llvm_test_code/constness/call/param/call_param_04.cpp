/* immutable: - */
void foo(int* a) {
	*a += 42;
}

int main() {
	int i = 10;
	foo(&i);
	return 0;
}