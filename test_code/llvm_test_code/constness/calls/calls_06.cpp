/* mutable: - */
void addTen(int a) {
	a += 10;
}

int main() {
	int a = 10;
	a = 20;
	addTen(a);
	return 0;
}