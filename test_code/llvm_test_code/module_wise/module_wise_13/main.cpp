#include "src1.h"

A *makeObj() {
	return new A;
}

int main(int argc, char **argv) {
	A *a = makeObj();
	int v = a->id(argc);
	// delete a;
	return 0;
}
