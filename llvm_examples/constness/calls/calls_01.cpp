/* mutable: f */
void foo() {
	int f = 0;
	f += 10;
}

int main() {
	foo();
	return 0;
}