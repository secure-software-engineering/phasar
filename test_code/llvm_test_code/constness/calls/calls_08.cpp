/* mutable: - */
void addTen(int &a) {
	a += 10;
}

void addOne(int &a) {
	a += 1;
}

int main() {
	int a = 10;
	int *p = &a;
	addTen(*p);
	addOne(*p);
	return 0;
}