int main(int argc, char **argv) {
	char *p = argv[0];
	int i = 42;
	int *j = &i;
	*j += 13;
	return i;
}