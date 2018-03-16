/* immutable: i */
void foo(int a) {
	int b = 42;
	b += a;
}

int main() {
	int i = 13;
	foo(i);
	return 0;
}