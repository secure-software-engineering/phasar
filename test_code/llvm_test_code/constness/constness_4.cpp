struct X {
	int i = 0;
};

int main() {
	// x is not const
	X x;
	X y;
	x = y;
	return 0;
}
