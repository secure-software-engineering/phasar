extern int source();     // dummy source
extern void sink(int p); // dummy sink

int main(int argc, char **argv) {
	int a = source();
	sink(a);
	return 0;
}