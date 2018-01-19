/* mutable: a */
void foo(int &a, int b) {
	a += b;
}

int main() {
	int a = 10;
	int b = 30;
	foo(a, b);
	return 0;
}