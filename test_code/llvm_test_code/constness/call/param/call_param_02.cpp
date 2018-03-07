/* immutable: i */
void foo(int a) {
	a += 42;
}

int main() {
	int i = 13;
	foo(i);
	return 0;
}