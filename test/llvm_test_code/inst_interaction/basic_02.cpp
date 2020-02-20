int main(int argc, char **argv) {
	int i = 13;
    int j = i + 42;
	if (argc > 1) {
		i = 42;
	} else {
		i = 666;
	}
	int k = i;
	return k;
}
