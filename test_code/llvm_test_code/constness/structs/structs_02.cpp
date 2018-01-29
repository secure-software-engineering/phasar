/* mutable: x */
struct A {
	int i = 0;
};

int main() {
	A x;
	A y;
	x = y;
	return 0;
}