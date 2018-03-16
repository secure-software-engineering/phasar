/* immutable: x */
struct A {
	int i = 10;
};

int main() {
	A x;
  A y;
  y.i = 20;
	return 0;
}