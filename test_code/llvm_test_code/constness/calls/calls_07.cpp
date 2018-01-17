/* mutable: - */
void addTen(int &a) {
	a += 10;
}

int main() {
	int a = 10;
	int *p = &a;
	addTen(*p);
	return 0;
}