/* immutable: j,b */
void foo(int &a, int b) {
	a += b;
}

int main() {
	int i = 10;
	int j = 30;
	foo(i, j);
	return 0;
}