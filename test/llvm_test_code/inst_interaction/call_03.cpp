unsigned factorial(unsigned n) {
	if (n <= 1) {
		return 1;
	}
	return n * factorial(n - 1);
}

int main() {
	int i = 7;
	int j = factorial(i);
	return j;
}
