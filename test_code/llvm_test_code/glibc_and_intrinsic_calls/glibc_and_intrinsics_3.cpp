#include <cstdlib>
#include <cstring>

int main() {
	int* i = (int*) malloc(10*sizeof(int));
	memset(i, 0, 10*sizeof(int));
	free(i);
	return 0;
}

