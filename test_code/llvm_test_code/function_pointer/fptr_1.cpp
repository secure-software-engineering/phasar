int add(int a, int b) {
	return a + b;
}

int main() {
	int a = 4;
	int b = 5;
	int (*f)(int, int) = &add;
	int c = f(a, b);
	return 0;
}