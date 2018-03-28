/* g, i | @g, %1 (ID: 0, 1) | mem2reg */
int *g;
int main() {
	int i = 42;
	g = &i;
	*g = 99;
	return 0;
}