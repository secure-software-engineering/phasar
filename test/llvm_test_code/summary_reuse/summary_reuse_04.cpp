int foo(int a) {
	return a + 2;
}

int main() {
	int i = 5;
	int b = 0;
	while(i-- > 0) {
		b += foo(i);
	}
}