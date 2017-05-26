int main() {
	int j = 13;
	int k = j + 10;
	// p is not const
	int* p = new int(k);
	*p = 100;
	delete p;
	return 0;
};
