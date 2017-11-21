int main() {
	int j = 13;
	int k = j + 10;
	// p is const
	int* p = new int(k);
	delete p;
	return 0;
};
