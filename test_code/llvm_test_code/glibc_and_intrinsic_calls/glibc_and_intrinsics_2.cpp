#include <cstdlib>
#include <cstring>


int foo() {
	return 42;
}

int bar() {
	return 13;
}

int main() {
	int* i = (int*) malloc(10*sizeof(int));
	memset(i, 0, 10*sizeof(int));
	free(i);

	int* j = new int(13);
	*j = foo();
	*j += bar();
	delete j;
	return 0;
}

