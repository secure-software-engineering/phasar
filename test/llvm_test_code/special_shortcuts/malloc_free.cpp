#include <stdlib.h>

int main() {
	int *mem = (int*) malloc(10 * sizeof(int));
	free(mem);
	return 0;
}