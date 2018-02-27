/* mutable: i (a) */
void foo(int &a) {
	a += 42;
}

int main() {
	int i = 10;
	int *p = &i;
	foo(*p);
	return 0;
}