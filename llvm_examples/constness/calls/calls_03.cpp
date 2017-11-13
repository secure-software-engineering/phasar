//#include <iostream>
/* mutable: i */
int addTen(int a) {
	int b;
	return a + 10;
}

int main() {
	int i = 10;
	addTen(i);
//	std::cout << i;
	return 0;
}