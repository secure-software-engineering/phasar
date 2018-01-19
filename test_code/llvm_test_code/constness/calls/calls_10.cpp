/* mutable: a,b */
void foo(int &a, int b) {
	a += b;
}

int main() {
	int a = 10;
	int b = 0;
	b = a + a;
	foo(a, b);
	return 0;
}